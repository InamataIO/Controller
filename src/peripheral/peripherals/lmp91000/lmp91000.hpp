#pragma once

#include <ArduinoJson.h>
#include <LMP91000.h>

#include <chrono>
#include <vector>

#include "managers/service_getters.h"
#include "peripheral/capabilities/get_values.h"
#include "peripheral/capabilities/start_measurement.h"
#include "peripheral/peripherals/i2c/i2c_abstract_peripheral.h"
#include "utils/coop_mutex.h"
#include "utils/uuid.h"

// TODO: Fork LMP91000.h and allow Wire* use

namespace inamata {
namespace peripheral {
namespace peripherals {
namespace lmp91000 {

class LMP91000 : public i2c::I2CAbstractPeripheral,
                 public capabilities::StartMeasurement,
                 public capabilities::GetValues {
 public:
  LMP91000(const JsonObjectConst& parameters);
  virtual ~LMP91000() = default;

  // Type registration in the peripheral factory
  const String& getType() const final;
  static const String& type();

  capabilities::StartMeasurement::Result startMeasurement(
      const JsonVariantConst& parameters) final;
  capabilities::StartMeasurement::Result handleMeasurement() final;
  capabilities::GetValues::Result getValues() final;

  /**
   * Overrides the JSON-configured conversion factor
   *
   * Used by O2Calibration to apply a reference-calibrated factor, either
   * freshly computed or restored from flash on boot.
   *
   * \param conversion_factor data_point_type_'s unit per uA of sensor current
   */
  void setConversionFactor(float conversion_factor);

 private:
  enum class InitState {
    kPreInit,
    kInitSelect,
    kInitConfig,
    kInitWait,
    kReady,
  };

  static std::shared_ptr<Peripheral> factory(const ServiceGetters& services,
                                             const JsonObjectConst& parameters);
  static bool registered_;
  static bool capability_start_measurement_;
  static bool capability_get_values_;

  std::chrono::milliseconds checkInitProbe();
  std::chrono::milliseconds initProbe();
  capabilities::StartMeasurement::Result sumInput();

  utils::UUID data_point_type_;
  uint8_t input_pin_;
  uint8_t config_enable_pin_;
  InitState init_state_;
  std::chrono::steady_clock::time_point state_entry_time_;
  float vzero_V_;
  float r_tia_ohm_;          // transimpedance gain
  float conversion_factor_;  // data_point_type_'s unit per uA of sensor current
  uint8_t gain_;
  uint8_t r_load_;
  uint8_t int_z_;
  uint8_t bias_;

  uint32_t input_sum_ = 0;
  uint8_t sum_count_ = 0;
  // Guards against a second task's startMeasurement() resetting an
  // in-progress accumulation started by another task sharing this instance
  bool measurement_active_ = false;
  // TODO: increase to 64
  static const uint8_t sum_iterations_ = 3;
  ::LMP91000 driver_;

  std::chrono::steady_clock::time_point now_;
  static utils::CoopMutex init_mutex_;
  /// Enable pins of already-configured chips; all share I2C address 0x48, so
  /// these are pulled high while another chip's registers are being written
  static std::vector<uint8_t> ready_enable_pins_;
  static constexpr std::chrono::milliseconds init_select_delay_{100};
  static constexpr std::chrono::milliseconds init_config_delay_{1000};
  static constexpr std::chrono::milliseconds init_wait_delay_{100};
  static constexpr std::chrono::milliseconds init_lock_retry_delay_{50};
};

}  // namespace lmp91000
}  // namespace peripherals
}  // namespace peripheral
}  // namespace inamata