#ifdef DEVICE_TYPE_MULTI_AIR_SENSOR
#include "auto_calibrate.h"

#include "managers/logging.h"
#include "managers/services.h"
#include "peripheral/fixed.h"
#include "utils/chrono.h"

namespace inamata {
namespace tasks {
namespace fixed {

AutoCalibrate::AutoCalibrate(const ServiceGetters& services,
                             Scheduler& scheduler,
                             const JsonObjectConst& behavior_config)
    : BaseTask(scheduler, Input(nullptr, true)) {
  // device_config is the JSON stored in LittleFS
  JsonDocument fixed_config_doc;
  Storage::loadFixedConfig(fixed_config_doc);

  JsonObjectConst task_config = findConfig(fixed_config_doc["tasks"]);
  if (task_config.isNull()) {
    // Auto calibration is optional, do nothing if it is not configured
    return;
  }

  JsonVariantConst check_interval_s = task_config[check_interval_s_key_];
  if (check_interval_s.is<uint32_t>()) {
    check_interval_ = std::chrono::seconds(check_interval_s.as<uint32_t>());
  }

  auto& peripheral_controller = Services::getPeripheralController();
  JsonArrayConst peripherals = task_config[peripherals_key_];
  String peripheral_error("Peripheral not found: ");
  for (JsonObjectConst peripheral_config : peripherals) {
    utils::UUID uuid(peripheral_config[peripheral_uuid_key_]);
    if (!uuid.isValid()) {
      setInvalid("Missing peripheral UUID");
      return;
    }
    auto peripheral = peripheral_controller.getPeripheral(uuid);
    if (peripheral == nullptr) {
      setInvalid(peripheral_error + uuid.toString());
      return;
    }
    auto calibrate_peripheral =
        std::dynamic_pointer_cast<Calibrate>(peripheral);
    if (calibrate_peripheral == nullptr) {
      setInvalid(Calibrate::invalidTypeError(uuid, peripheral));
      return;
    }
    Target target;
    target.peripheral = calibrate_peripheral;
    targets_.push_back(target);
  }

  if (targets_.empty()) {
    return;
  }

  setIterations(TASK_FOREVER);
  enable();
}

const String& AutoCalibrate::getType() const { return type(); }

const String& AutoCalibrate::type() {
  static const String name{"AutoCalibrate"};
  return name;
}

JsonObjectConst AutoCalibrate::findConfig(JsonArrayConst tasks_config) {
  const String& type_name = type();
  for (JsonObjectConst task : tasks_config) {
    if (task["type"] == type_name) {
      return task;
    }
  }
  return JsonObjectConst();
}

bool AutoCalibrate::TaskCallback() {
  std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
  std::chrono::nanoseconds min_wait = std::chrono::nanoseconds::max();

  for (Target& target : targets_) {
    if (target.wait_until <= now) {
      if (target.state == Target::State::kWaiting) {
        Calibrate::Result result =
            target.peripheral->startCalibration(JsonObjectConst());
        if (result.error.isError()) {
          TRACEF("Error startCalibration: %s\r\n",
                 result.error.toString().c_str());
          target.wait_until = now + check_interval_;
        } else if (result.wait == std::chrono::nanoseconds::zero()) {
          // Nothing to calibrate this round, wait for the next check
          target.wait_until = now + check_interval_;
        } else {
          target.state = Target::State::kCalibrating;
          target.wait_until = now + result.wait;
        }
      } else {
        Calibrate::Result result = target.peripheral->handleCalibration();
        if (result.error.isError()) {
          TRACEF("Error handleCalibration: %s\r\n",
                 result.error.toString().c_str());
          target.state = Target::State::kWaiting;
          target.wait_until = now + check_interval_;
        } else if (result.wait == std::chrono::nanoseconds::zero()) {
          // Calibration candidate was applied (or none was found), start the
          // next check after the configured interval
          target.state = Target::State::kWaiting;
          target.wait_until = now + check_interval_;
        } else {
          target.wait_until = now + result.wait;
        }
      }
    }

    std::chrono::nanoseconds wait = target.wait_until > now
                                        ? target.wait_until - now
                                        : std::chrono::nanoseconds::zero();
    if (wait < min_wait) {
      min_wait = wait;
    }
  }

  if (min_wait == std::chrono::nanoseconds::max()) {
    Task::delay(std::chrono::milliseconds(default_interval_).count());
  } else {
    int64_t wait =
        std::chrono::duration_cast<std::chrono::milliseconds>(min_wait).count();
    Task::delay(wait);
  }

  return true;
}

const __FlashStringHelper* AutoCalibrate::peripherals_key_ =
    FPSTR("peripherals");
const __FlashStringHelper* AutoCalibrate::peripheral_uuid_key_ = FPSTR("uuid");
const __FlashStringHelper* AutoCalibrate::check_interval_s_key_ =
    FPSTR("check_interval_s");

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata

#endif
