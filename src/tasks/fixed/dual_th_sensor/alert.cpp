#ifdef DEVICE_TYPE_DUAL_TH_SENSOR

#include "alert.h"

#include <algorithm>
#include <functional>

#include "peripheral/fixed.h"
#include "utils/chrono.h"

namespace inamata {
namespace tasks {
namespace fixed {

using namespace std::placeholders;

std::array<Alert::LimitInfo, 8> Alert::makeLimits(const char* side) {
  const utils::UUID temperature(peripheral::fixed::dpt_temperature_c_id);
  const utils::UUID humidity(peripheral::fixed::dpt_humidity_rh_id);
  const String temp_prefix = String(side) + "_temperature_";
  const String humidity_prefix = String(side) + "_humidity_";

  return {{{temp_prefix + "low_2", temperature, LimitInfo::Condition::kLessThan,
            LimitInfo::Severity::kDanger},
           {temp_prefix + "low_1", temperature, LimitInfo::Condition::kLessThan,
            LimitInfo::Severity::kWarning},
           {temp_prefix + "high_1", temperature,
            LimitInfo::Condition::kGreaterThan, LimitInfo::Severity::kWarning},
           {temp_prefix + "high_2", temperature,
            LimitInfo::Condition::kGreaterThan, LimitInfo::Severity::kDanger},
           {humidity_prefix + "low_2", humidity,
            LimitInfo::Condition::kLessThan, LimitInfo::Severity::kDanger},
           {humidity_prefix + "low_1", humidity,
            LimitInfo::Condition::kLessThan, LimitInfo::Severity::kWarning},
           {humidity_prefix + "high_1", humidity,
            LimitInfo::Condition::kGreaterThan, LimitInfo::Severity::kWarning},
           {humidity_prefix + "high_2", humidity,
            LimitInfo::Condition::kGreaterThan, LimitInfo::Severity::kDanger}}};
}

Alert::Alert(const ServiceGetters& services, Scheduler& scheduler,
             const JsonObjectConst& behavior_config)
    : BaseTask(scheduler, Input(nullptr, true)),
      sensors_{{{nullptr, makeLimits("l")}, {nullptr, makeLimits("r")}}},
      web_socket_(services.getWebSocket()) {
  if (!isValid()) {
    return;
  }
  if (web_socket_ == nullptr) {
    setInvalid(services.web_socket_nullptr_error_);
    return;
  }
  if (!setFixedPeripherals()) {
    return;
  }

  if (!behavior_config.isNull()) {
    handleBehaviorConfig(behavior_config);
  }
  Services::getBehaviorController().registerConfigCallback(
      std::bind(&Alert::handleBehaviorConfig, this, _1));

  status_led_->turnOn(ok_color_);
  Task::setInterval(std::chrono::milliseconds(default_interval_).count());
  setIterations(TASK_FOREVER);
  enable();
}

const String& Alert::getType() const { return type(); }

const String& Alert::type() {
  static const String name{"DualThAlert"};
  return name;
}

bool Alert::TaskCallback() {
  const auto now = std::chrono::steady_clock::now();
  std::chrono::nanoseconds min_wait = std::chrono::nanoseconds::max();
  bool waiting = false;
  for (SensorInfo& sensor : sensors_) {
    if (sensor.cycle_complete) {
      continue;
    }
    auto measurement =
        sensor.measurement_active
            ? sensor.peripheral->handleMeasurement()
            : sensor.peripheral->startMeasurement(JsonVariantConst());
    if (measurement.error.isError()) {
      sensor.measurement_active = false;
      sensor.cycle_complete = true;
      continue;
    }
    if (measurement.wait != std::chrono::nanoseconds::zero()) {
      sensor.measurement_active = true;
      waiting = true;
      min_wait = std::min(min_wait, measurement.wait);
      continue;
    }

    sensor.measurement_active = false;
    sensor.cycle_complete = true;
    auto result = sensor.peripheral->getValues();
    if (result.error.isError()) {
      continue;
    }
    for (const utils::ValueUnit& value : result.values) {
      for (LimitInfo& limit : sensor.limits) {
        if (limit.data_point_type == value.data_point_type) {
          handleLimit(limit, value, sensor.peripheral->id, now);
        }
      }
    }
  }
  std::chrono::milliseconds next_delay;
  if (waiting) {
    const auto wait_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(min_wait).count();
    next_delay = std::chrono::milliseconds(std::max<int64_t>(1, wait_ms));
  } else {
    for (SensorInfo& sensor : sensors_) {
      sensor.cycle_complete = false;
    }
    next_delay = std::chrono::duration_cast<std::chrono::milliseconds>(
        default_interval_);
  }

  next_delay = std::min(next_delay, updateLed(now));
  Task::delay(next_delay.count());
  return true;
}

void Alert::handleBehaviorConfig(const JsonObjectConst& config) {
  for (SensorInfo& sensor : sensors_) {
    for (LimitInfo& limit : sensor.limits) {
      setLimitConfig(limit, config[limit.config_key], sensor.peripheral->id);
    }
  }
}

void Alert::setLimitConfig(LimitInfo& limit, JsonVariantConst config,
                           const utils::UUID& peripheral_id) {
  if (config.isNull()) {
    limit.enabled = false;
    return;
  }

  const utils::UUID limit_id(config[WebSocket::limit_id_key_]);
  if (limit_id.isValid() && limit_id != limit.limit_id) {
    if (limit.crossed) {
      // Close the existing crossing before changing its event identity.
      sendLimitEvent(limit,
                     utils::ValueUnit(limit.last_value, limit.data_point_type),
                     peripheral_id, utils::LimitEvent::Type::kEnd);
    }
    limit.crossed = false;
    limit.delay_start = std::chrono::steady_clock::time_point::min();
    limit.last_continue = std::chrono::steady_clock::time_point::min();
    limit.limit_id = limit_id;
  }

  const JsonVariantConst threshold = config["limit"];
  if (threshold.is<float>()) {
    limit.threshold = threshold.as<float>();
  }

  const JsonVariantConst delay = config["delay_s"];
  if (delay.is<float>()) {
    limit.delay = std::chrono::milliseconds(
        static_cast<int64_t>(delay.as<float>() * 1000));
  }

  const JsonVariantConst enabled = config["enabled"];
  limit.enabled =
      enabled.is<bool>() ? enabled.as<bool>() : limit.limit_id.isValid();
}

void Alert::handleLimit(LimitInfo& limit, const utils::ValueUnit& value,
                        const utils::UUID& peripheral_id,
                        const std::chrono::steady_clock::time_point now) {
  limit.last_value = value.value;
  const bool crossed = limit.enabled && !std::isnan(limit.threshold) &&
                       (limit.condition == LimitInfo::Condition::kLessThan
                            ? value.value < limit.threshold
                            : value.value > limit.threshold);
  if (!crossed) {
    limit.delay_start = std::chrono::steady_clock::time_point::min();
    if (limit.crossed) {
      sendLimitEvent(limit, value, peripheral_id,
                     utils::LimitEvent::Type::kEnd);
      limit.crossed = false;
    }
    return;
  }

  if (limit.delay > std::chrono::milliseconds::zero()) {
    if (limit.delay_start == std::chrono::steady_clock::time_point::min()) {
      limit.delay_start = now;
      return;
    }
    if (!utils::hasTimedOut(limit.delay_start, limit.delay, now)) {
      return;
    }
  }

  if (!limit.crossed) {
    sendLimitEvent(limit, value, peripheral_id,
                   utils::LimitEvent::Type::kStart);
    limit.last_continue = now;
    limit.crossed = true;
  } else if (utils::hasTimedOut(limit.last_continue, continue_event_period_,
                                now)) {
    sendLimitEvent(limit, value, peripheral_id,
                   utils::LimitEvent::Type::kContinue);
    limit.last_continue = now;
  }
}

void Alert::sendLimitEvent(const LimitInfo& limit,
                           const utils::ValueUnit& value,
                           const utils::UUID& peripheral_id,
                           const utils::LimitEvent::Type event_type) {
  if (!limit.limit_id.isValid()) {
    return;
  }

  JsonDocument event;
  if (Services::is_time_synced_) {
    event[WebSocket::time_key_] = utils::getIsoTimestamp();
  }
  event[WebSocket::limit_id_key_] = limit.limit_id.toString();
  event[utils::ValueUnit::value_key] = value.value;
  event[WebSocket::fixed_peripheral_id_key_] = peripheral_id.toString();
  event[WebSocket::fixed_dpt_id_key_] = value.data_point_type.toString();
  switch (event_type) {
    case utils::LimitEvent::Type::kStart:
      event[utils::LimitEvent::event_type_key_] =
          utils::LimitEvent::start_type_;
      break;
    case utils::LimitEvent::Type::kContinue:
      event[utils::LimitEvent::event_type_key_] =
          utils::LimitEvent::continue_type_;
      break;
    case utils::LimitEvent::Type::kEnd:
      event[utils::LimitEvent::event_type_key_] = utils::LimitEvent::end_type_;
      break;
  }
  web_socket_->sendLimitEvent(event.as<JsonObject>());
}

void Alert::showLimitStatus() {
  bool warning = false;
  for (const SensorInfo& sensor : sensors_) {
    for (const LimitInfo& limit : sensor.limits) {
      if (!limit.crossed) {
        continue;
      }
      if (limit.severity == LimitInfo::Severity::kDanger) {
        status_led_->turnOn(danger_color_);
        return;
      }
      warning = true;
    }
  }
  status_led_->turnOn(warning ? warning_color_ : ok_color_);
}

std::chrono::milliseconds Alert::updateLed(
    const std::chrono::steady_clock::time_point now) {
  const bool missing_probe = std::any_of(
      sensors_.begin(), sensors_.end(), [](const SensorInfo& sensor) {
        return !sensor.peripheral->isInitialized();
      });

  if (!missing_probe) {
    missing_probe_blink_active_ = false;
    missing_probe_blink_start_ = std::chrono::steady_clock::time_point::min();
    showLimitStatus();
    return std::chrono::milliseconds::max();
  }

  if (missing_probe_blink_start_ ==
      std::chrono::steady_clock::time_point::min()) {
    missing_probe_blink_start_ = now;
  }

  const auto since_blink_start =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          now - missing_probe_blink_start_);
  if (missing_probe_blink_active_) {
    if (since_blink_start < missing_probe_blink_duration_) {
      status_led_->turnOff();
      return missing_probe_blink_duration_ - since_blink_start;
    }
    missing_probe_blink_active_ = false;
    showLimitStatus();
  }

  if (since_blink_start >= missing_probe_blink_period_) {
    missing_probe_blink_active_ = true;
    missing_probe_blink_start_ = now;
    status_led_->turnOff();
    return missing_probe_blink_duration_;
  }

  showLimitStatus();
  return std::chrono::duration_cast<std::chrono::milliseconds>(
      missing_probe_blink_period_ - since_blink_start);
}

bool Alert::setFixedPeripherals() {
  const char* sensor_ids[] = {peripheral::fixed::peripheral_air_left_id,
                              peripheral::fixed::peripheral_air_right_id};
  const String& sensor_type =
      peripheral::peripherals::sht2x_sensor::SHT2xSensor::type();
  for (size_t i = 0; i < sensors_.size(); ++i) {
    const utils::UUID id(sensor_ids[i]);
    auto peripheral = Services::getPeripheralController().getPeripheral(id);
    if (!peripheral) {
      setInvalid(peripheral::Peripheral::peripheralNotFoundError(id));
      return false;
    }
    if (!peripheral->isValid() || peripheral->getType() != sensor_type) {
      setInvalid(peripheral::Peripheral::notAValidError(id, sensor_type));
      return false;
    }
    sensors_[i].peripheral = std::static_pointer_cast<
        peripheral::peripherals::sht2x_sensor::SHT2xSensor>(peripheral);
  }

  const utils::UUID led_id(peripheral::fixed::peripheral_status_led_id);
  auto peripheral = Services::getPeripheralController().getPeripheral(led_id);
  if (!peripheral) {
    setInvalid(peripheral::Peripheral::peripheralNotFoundError(led_id));
    return false;
  }
  const String& led_type = peripheral::peripherals::neo_pixel::NeoPixel::type();
  if (!peripheral->isValid() || peripheral->getType() != led_type) {
    setInvalid(peripheral::Peripheral::notAValidError(led_id, led_type));
    return false;
  }
  status_led_ =
      std::static_pointer_cast<peripheral::peripherals::neo_pixel::NeoPixel>(
          peripheral);
  return true;
}

const std::chrono::seconds Alert::default_interval_{1};

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata

#endif
