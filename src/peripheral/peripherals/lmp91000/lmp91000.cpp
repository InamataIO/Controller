#include "lmp91000.hpp"

#include <algorithm>
#include <chrono>

#include "ArduinoJson/Object/JsonObjectConst.hpp"
#include "managers/types.h"
#include "peripheral/peripheral_factory.h"
#include "peripheral/peripherals/i2c/i2c_abstract_peripheral.h"
#include "utils/chrono.h"

namespace inamata {
namespace peripheral {
namespace peripherals {
namespace lmp91000 {

LMP91000::LMP91000(const JsonObjectConst& parameters)
    : i2c::I2CAbstractPeripheral(parameters) {
  // If the base class constructor failed, abort the constructor
  if (!isValid()) {
    return;
  }

  if (getWireIndex() != 0) {
    setInvalid("Only Wire0 supported");
    return;
  }

  vzero_V_ = parameters["vzero_V"];

  analogReadResolution(12);

  const uint8_t input_pin = parameters["input_pin"];
  const uint8_t config_enable_pin = parameters["config_enable_pin"];

  config_enable_pin_ = config_enable_pin;
  driver_.setMENB(config_enable_pin);
  driver_.disable();

  pinMode(input_pin, INPUT);
  input_pin_ = input_pin;

  data_point_type_ = parameters["data_point_type"];

  r_tia_ohm_ = parameters["r_tia_ohm"];
  conversion_factor_ = parameters["conversion_factor"];
  gain_ = parameters["gain"];
  r_load_ = parameters["r_load"];
  int_z_ = parameters["int_z"];
  bias_ = parameters["bias"];
  Serial.printf(
      "dtp: %s, config: %d, input: %d, rtia: %f, conv: %f, gain: %d, rload: "
      "%d, intz: %d, bias: %d\r\n",
      data_point_type_.toString().c_str(), config_enable_pin_, input_pin_,
      r_tia_ohm_, conversion_factor_, gain_, r_load_, int_z_, bias_);
  init_state_ = InitState::kPreInit;
}

const String& LMP91000::getType() const { return type(); }

const String& LMP91000::type() {
  static const String name{"LMP91000"};
  return name;
}

capabilities::StartMeasurement::Result LMP91000::startMeasurement(
    const JsonVariantConst& parameters) {
  now_ = std::chrono::steady_clock::now();
  const std::chrono::milliseconds wait = checkInitProbe();
  if (wait != std::chrono::milliseconds::zero()) {
    return capabilities::StartMeasurement::Result(wait);
  }

  // Another task may already be mid-average on this shared instance; join it
  // instead of clobbering its accumulator
  if (!measurement_active_) {
    measurement_active_ = true;
    sum_count_ = 0;
    input_sum_ = 0;
  }

  return sumInput();
}

capabilities::StartMeasurement::Result LMP91000::handleMeasurement() {
  now_ = std::chrono::steady_clock::now();
  const std::chrono::milliseconds wait = checkInitProbe();
  if (wait != std::chrono::milliseconds::zero()) {
    return capabilities::StartMeasurement::Result(wait);
  }
  return sumInput();
}

capabilities::GetValues::Result LMP91000::getValues() {
  const float vout_V = input_sum_ / (sum_iterations_ * 1000.0);
  const float current_uA = (fabs(vzero_V_ - vout_V) / r_tia_ohm_) * 1000000.0;
  // Value is in whatever unit data_point_type_ defines (ppm, %, etc.); can't
  // be negative, so this also clamps any residual settling transient
  const float value = std::max(current_uA * conversion_factor_, 0.0f);
  // TRACEF("V: %f, µA: %f, value: %f\r\n", vout_V, current_uA, value);
  return capabilities::GetValues::Result(
      {utils::ValueUnit(value, data_point_type_)}, ErrorResult());
}

void LMP91000::setConversionFactor(float conversion_factor) {
  conversion_factor_ = conversion_factor;
}

std::chrono::milliseconds LMP91000::checkInitProbe() {
  if (init_state_ == InitState::kReady) {
    return std::chrono::milliseconds::zero();
  }

  // Acquire the global init lock only once when entering the init sequence.
  // Re-locking while already in an init state causes a self-deadlock loop.
  if (init_state_ == InitState::kPreInit && !init_mutex_.tryLock()) {
    return init_lock_retry_delay_;
  }

  return initProbe();
}

std::chrono::milliseconds LMP91000::initProbe() {
  std::chrono::nanoseconds diff;
  // Enter state, perform action, wait until delay passed
  switch (init_state_) {
    case InitState::kPreInit:
      // Quiesce already-configured chips (shared I2C address) before talking
      // to this one
      for (uint8_t pin : ready_enable_pins_) {
        digitalWrite(pin, HIGH);
      }
      digitalWrite(config_enable_pin_, LOW);

      init_state_ = InitState::kInitSelect;
      state_entry_time_ = now_;
      return init_select_delay_;
    case InitState::kInitSelect:
      diff = now_ - (state_entry_time_ + init_select_delay_);
      if (diff < std::chrono::nanoseconds::zero()) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(-diff);
      }

      driver_.enable();
      driver_.disableFET();
      driver_.setGain(gain_);
      driver_.setRLoad(r_load_);
      driver_.setIntRefSource();
      driver_.setIntZ(int_z_);
      driver_.setBias(bias_);
      driver_.setThreeLead();  // 3-electrode mode

      init_state_ = InitState::kInitConfig;
      state_entry_time_ = now_;
      return init_config_delay_;
    case InitState::kInitConfig:
      diff = now_ - (state_entry_time_ + init_config_delay_);
      if (diff < std::chrono::nanoseconds::zero()) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(-diff);
      }

      // Re-enable other chips; keep this one enabled too so its TIA keeps
      // driving VOUT for analogRead() in sumInput()
      for (uint8_t pin : ready_enable_pins_) {
        digitalWrite(pin, LOW);
      }
      ready_enable_pins_.push_back(config_enable_pin_);

      // Bus is safe again; release the lock now so this chip's settle wait
      // below doesn't block other chips from starting their own init
      init_mutex_.unlock();

      init_state_ = InitState::kInitWait;
      state_entry_time_ = now_;
      return init_wait_delay_;
    case InitState::kInitWait:
      diff = now_ - (state_entry_time_ + init_wait_delay_);
      if (diff < std::chrono::nanoseconds::zero()) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(-diff);
      }

      init_state_ = InitState::kReady;
      state_entry_time_ = now_;
      break;
    default:
      break;
  }
  return std::chrono::milliseconds::zero();
}

capabilities::StartMeasurement::Result LMP91000::sumInput() {
  if (sum_count_ >= sum_iterations_) {
    measurement_active_ = false;
    return capabilities::StartMeasurement::Result();
  }
  input_sum_ += analogReadMilliVolts(input_pin_);
  sum_count_++;
  if (sum_count_ >= sum_iterations_) {
    measurement_active_ = false;
    return capabilities::StartMeasurement::Result();
  } else {
    return capabilities::StartMeasurement::Result(std::chrono::milliseconds(2));
  }
}

std::shared_ptr<Peripheral> LMP91000::factory(
    const ServiceGetters& services, const JsonObjectConst& parameters) {
  return std::make_shared<LMP91000>(parameters);
}

bool LMP91000::registered_ =
    PeripheralFactory::registerFactory(type(), factory);

bool LMP91000::capability_start_measurement_ =
    capabilities::StartMeasurement::registerType(type());

bool LMP91000::capability_get_values_ =
    capabilities::GetValues::registerType(type());

utils::CoopMutex LMP91000::init_mutex_ = utils::CoopMutex();
std::vector<uint8_t> LMP91000::ready_enable_pins_;

}  // namespace lmp91000
}  // namespace peripherals
}  // namespace peripheral
}  // namespace inamata