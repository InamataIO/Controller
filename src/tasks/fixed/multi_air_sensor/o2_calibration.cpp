#ifdef DEVICE_TYPE_MULTI_AIR_SENSOR

#include "o2_calibration.h"

#include "aging.h"
#include "peripheral/peripheral.h"
#include "utils/chrono.h"

namespace inamata {
namespace tasks {
namespace fixed {

namespace {
const char* o2_calibration_key = "o2_calibration";
}

O2Calibration::O2Calibration(const ServiceGetters& services,
                             Scheduler& scheduler,
                             const JsonObjectConst& behavior_config)
    : BaseTask(scheduler, Input(nullptr, true)) {
  JsonDocument fixed_config_doc;
  Storage::loadFixedConfig(fixed_config_doc);

  JsonObjectConst calib_config;
  for (JsonObjectConst peripheral_config :
       fixed_config_doc["peripherals"].as<JsonArrayConst>()) {
    if (peripheral_config[o2_calibration_key] | false) {
      calib_config = peripheral_config;
      break;
    }
  }
  // No sensor configured for O2 calibration on this device; enable once so
  // TaskCallback can disable/remove this task via the normal TaskRemovalTask
  // flow instead of leaking the object
  if (calib_config.isNull()) {
    setIterations(TASK_FOREVER);
    enable();
    return;
  }

  uuid_ = utils::UUID(calib_config["uuid"]);
  configured_conversion_factor_ = calib_config["conversion_factor"] | 0.0f;

  auto peripheral = Services::getPeripheralController().getPeripheral(uuid_);
  if (peripheral == nullptr) {
    setInvalid(peripheral::Peripheral::peripheralNotFoundError(uuid_));
    return;
  }
  peripheral_ = std::dynamic_pointer_cast<LMP91000>(peripheral);
  if (peripheral_ == nullptr) {
    setInvalid(peripheral::Peripheral::notAValidError(uuid_, LMP91000::type()));
    return;
  }

  // If already calibrated, apply the stored factor and skip recalibration
  JsonDocument calibration_doc;
  Storage::loadCalibration(calibration_doc);
  JsonVariantConst stored_factor =
      calibration_doc["conversion_factor"][uuid_.toString()];
  if (!stored_factor.isNull()) {
    peripheral_->setConversionFactor(stored_factor.as<float>());
    state_ = State::kDone;
  }

  setIterations(TASK_FOREVER);
  enable();
}

const String& O2Calibration::getType() const { return type(); }

const String& O2Calibration::type() {
  static const String name{"O2Calibration"};
  return name;
}

bool O2Calibration::TaskCallback() {
  // No sensor was configured for O2 calibration; remove this task
  if (peripheral_ == nullptr) {
    return false;
  }

  if (state_ == State::kDone) {
    return false;
  }

  const auto now = std::chrono::steady_clock::now();

  if (state_ == State::kWaitAging) {
    if (Aging::getHoursRemaining(uuid_) > 0) {
      Task::delay(std::chrono::duration_cast<std::chrono::milliseconds>(
                      aging_check_interval_)
                      .count());
      return true;
    }
    sampling_start_ = now;
    state_ = State::kStartMeasurement;
  }

  if (state_ == State::kStartMeasurement) {
    StartMeasurement::Result result =
        peripheral_->startMeasurement(JsonVariantConst());
    if (result.error.isError()) {
      Task::delay(std::chrono::milliseconds(default_interval_).count());
      return true;
    }
    state_ = State::kHandleMeasurement;
    wait_until_ = now + result.wait;
  }

  if (state_ == State::kHandleMeasurement) {
    if (now < wait_until_) {
      Task::delay(std::chrono::duration_cast<std::chrono::milliseconds>(
                      wait_until_ - now)
                      .count());
      return true;
    }
    StartMeasurement::Result result = peripheral_->handleMeasurement();
    if (result.error.isError()) {
      Task::delay(std::chrono::milliseconds(default_interval_).count());
      return true;
    }
    if (result.wait != std::chrono::nanoseconds::zero()) {
      wait_until_ = now + result.wait;
      Task::delay(
          std::chrono::duration_cast<std::chrono::milliseconds>(result.wait)
              .count());
      return true;
    }
    state_ = State::kGetValues;
  }

  if (state_ == State::kGetValues) {
    GetValues::Result result = peripheral_->getValues();
    if (result.error.isError() || result.values.empty()) {
      Task::delay(std::chrono::milliseconds(default_interval_).count());
      return true;
    }
    value_sum_ += result.values.front().value;
    sample_count_++;

    // Keep sampling until the averaging window has elapsed
    if (!utils::hasTimedOut(sampling_start_, sampling_duration_, now)) {
      state_ = State::kStartMeasurement;
      Task::delay(std::chrono::duration_cast<std::chrono::milliseconds>(
                      sample_interval_)
                      .count());
      return true;
    }

    const float measured_value = value_sum_ / sample_count_;
    // Rescale the configured factor so the averaged reading matches the known
    // ambient O2 reference instead of assuming a raw current offset
    if (measured_value > 0 && configured_conversion_factor_ > 0) {
      const float new_factor = configured_conversion_factor_ *
                               (reference_o2_percent_ / measured_value);
      peripheral_->setConversionFactor(new_factor);
      storeConversionFactor(new_factor);
    }
    state_ = State::kDone;
  }

  Task::delay(std::chrono::milliseconds(default_interval_).count());
  return true;
}

void O2Calibration::storeConversionFactor(float conversion_factor) {
  JsonDocument calibration_doc;
  Storage::loadCalibration(calibration_doc);
  JsonObject calibration = calibration_doc.as<JsonObject>();
  if (calibration.isNull()) {
    calibration = calibration_doc.to<JsonObject>();
  }
  JsonObject conversion_factor_config =
      calibration["conversion_factor"].as<JsonObject>();
  if (conversion_factor_config.isNull()) {
    conversion_factor_config =
        calibration["conversion_factor"].to<JsonObject>();
  }
  conversion_factor_config[uuid_.toString()] = conversion_factor;
  Storage::storeCalibration(calibration);
}

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata

#endif
