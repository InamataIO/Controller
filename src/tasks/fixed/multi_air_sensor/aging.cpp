#ifdef DEVICE_TYPE_MULTI_AIR_SENSOR

#include "aging.h"

#include "utils/chrono.h"

namespace inamata {
namespace tasks {
namespace fixed {

Aging::Aging(const ServiceGetters& services, Scheduler& scheduler,
             const JsonObjectConst& behavior_config)
    : BaseTask(scheduler, Input(nullptr, true)) {
  JsonDocument fixed_config_doc;
  Storage::loadFixedConfig(fixed_config_doc);

  for (JsonObjectConst peripheral_config :
       fixed_config_doc["peripherals"].as<JsonArrayConst>()) {
    const uint16_t aging_hours = peripheral_config["aging_hours"] | 0;
    if (aging_hours == 0) {
      continue;
    }
    aging_sensors_.push_back({.uuid = utils::UUID(peripheral_config["uuid"]),
                              .aging_hours_target = aging_hours,
                              .aged_hours = 0});
  }

  if (!aging_sensors_.empty()) {
    // Resume each peripheral's elapsed-hours counter across reboots
    JsonDocument calibration_doc;
    Storage::loadCalibration(calibration_doc);
    JsonObjectConst aged_hours_config = calibration_doc["aged_hours"];
    uint16_t max_remaining = 0;
    for (AgingSensor& sensor : aging_sensors_) {
      sensor.aged_hours = aged_hours_config[sensor.uuid.toString()] | 0;
      const uint16_t remaining =
          sensor.aging_hours_target > sensor.aged_hours
              ? sensor.aging_hours_target - sensor.aged_hours
              : 0;
      if (remaining > max_remaining) {
        max_remaining = remaining;
      }
    }
    max_hours_remaining_ = max_remaining;
  }

  last_save_ = std::chrono::steady_clock::now();
  setIterations(TASK_FOREVER);
  enable();
}

const String& Aging::getType() const { return type(); }

const String& Aging::type() {
  static const String name{"Aging"};
  return name;
}

bool Aging::TaskCallback() {
  updateAging(std::chrono::steady_clock::now());
  Task::delay(
      std::chrono::duration_cast<std::chrono::milliseconds>(check_interval_)
          .count());
  return true;
}

void Aging::updateAging(std::chrono::steady_clock::time_point now) {
  if (aging_sensors_.empty() ||
      !utils::hasTimedOut(last_save_, save_interval_, now)) {
    return;
  }
  last_save_ += save_interval_;

  bool changed = false;
  for (AgingSensor& sensor : aging_sensors_) {
    if (sensor.aged_hours < sensor.aging_hours_target) {
      sensor.aged_hours++;
      changed = true;
    }
  }
  if (!changed) {
    return;
  }

  JsonDocument calibration_doc;
  Storage::loadCalibration(calibration_doc);
  JsonObject calibration = calibration_doc.as<JsonObject>();
  if (calibration.isNull()) {
    calibration = calibration_doc.to<JsonObject>();
  }
  JsonObject aged_hours_config = calibration["aged_hours"].as<JsonObject>();
  if (aged_hours_config.isNull()) {
    aged_hours_config = calibration["aged_hours"].to<JsonObject>();
  }

  uint16_t max_remaining = 0;
  for (const AgingSensor& sensor : aging_sensors_) {
    aged_hours_config[sensor.uuid.toString()] = sensor.aged_hours;
    const uint16_t remaining =
        sensor.aging_hours_target > sensor.aged_hours
            ? sensor.aging_hours_target - sensor.aged_hours
            : 0;
    if (remaining > max_remaining) {
      max_remaining = remaining;
    }
  }
  Storage::storeCalibration(calibration);
  max_hours_remaining_ = max_remaining;
}

uint16_t Aging::getMaxHoursRemaining() { return max_hours_remaining_; }

uint16_t Aging::getHoursRemaining(const utils::UUID& uuid) {
  for (const AgingSensor& sensor : aging_sensors_) {
    if (sensor.uuid == uuid) {
      return sensor.aging_hours_target > sensor.aged_hours
                 ? sensor.aging_hours_target - sensor.aged_hours
                 : 0;
    }
  }
  return 0;
}

uint16_t Aging::max_hours_remaining_ = 0;
std::vector<Aging::AgingSensor> Aging::aging_sensors_;

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata

#endif
