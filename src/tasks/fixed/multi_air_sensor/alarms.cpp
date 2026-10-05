#ifdef DEVICE_TYPE_MULTI_AIR_SENSOR

#include "alarms.h"

#include "utils/chrono.h"

namespace inamata {
namespace tasks {
namespace fixed {

using namespace std::placeholders;

namespace {
const char* data_point_type_suffix = "data_point_type";
}

Alarms::Alarms(const ServiceGetters& services, Scheduler& scheduler,
               const JsonObjectConst& behavior_config)
    : BaseTask(scheduler, Input(nullptr, true)),
      web_socket_(services.getWebSocket()) {
  if (!isValid()) {
    return;
  }
  if (web_socket_ == nullptr) {
    setInvalid(services.web_socket_nullptr_error_);
    return;
  }

  JsonDocument fixed_config_doc;
  Storage::loadFixedConfig(fixed_config_doc);

  auto& peripheral_controller = Services::getPeripheralController();
  String peripheral_error("Peripheral not found: ");

  for (JsonObjectConst peripheral_config :
       fixed_config_doc["peripherals"].as<JsonArrayConst>()) {
    if (!peripheral_config["register"].as<bool>()) {
      continue;
    }

    // Match every "*data_point_type" key with its sibling "*limits" array so
    // peripherals with several DPTs (e.g. SCD4X) can have limits per DPT
    std::vector<DptLimits> dpt_limits;
    for (JsonPairConst field : peripheral_config) {
      String key(field.key().c_str());
      if (!key.endsWith(data_point_type_suffix)) {
        continue;
      }
      String prefix =
          key.substring(0, key.length() - strlen(data_point_type_suffix));
      JsonArrayConst limit_names =
          peripheral_config[prefix + "limits"].as<JsonArrayConst>();
      if (limit_names.isNull() || limit_names.size() == 0) {
        continue;
      }

      DptLimits entry;
      entry.dpt = utils::UUID(field.value());
      for (JsonVariantConst limit_name : limit_names) {
        LimitInfo limit;
        limit.config_key = limit_name.as<const char*>();
        entry.limits.push_back(limit);
      }
      dpt_limits.push_back(std::move(entry));
    }
    if (dpt_limits.empty()) {
      // Peripheral is registered for telemetry, but has no alarms configured
      continue;
    }

    utils::UUID uuid(peripheral_config["uuid"]);
    auto peripheral = peripheral_controller.getPeripheral(uuid);
    if (peripheral == nullptr) {
      setInvalid(peripheral_error + uuid.toString());
      return;
    }
    auto get_values_peripheral =
        std::dynamic_pointer_cast<GetValues>(peripheral);
    if (get_values_peripheral == nullptr) {
      setInvalid(GetValues::invalidTypeError(uuid, peripheral));
      return;
    }

    sensors_.push_back(
        {.peripheral = peripheral, .dpt_limits = std::move(dpt_limits)});
  }

  if (!behavior_config.isNull()) {
    handleBehaviorConfig(behavior_config);
  }
  Services::getBehaviorController().registerConfigCallback(
      std::bind(&Alarms::handleBehaviorConfig, this, _1));

  setIterations(TASK_FOREVER);
  enable();
}

const String& Alarms::getType() const { return type(); }

const String& Alarms::type() {
  static const String name{"SensorAlarm"};
  return name;
}

bool Alarms::TaskCallback() {
  const std::chrono::steady_clock::time_point now =
      std::chrono::steady_clock::now();
  std::chrono::nanoseconds min_wait = std::chrono::nanoseconds::max();

  // Start new measurements for all sensors that support the capability
  if (utils::hasTimedOut(last_check_, check_interval_, now)) {
    last_check_ = now;

    for (Sensor& sensor : sensors_) {
      sensor.wait_until = now;
      auto measure_sensor =
          std::dynamic_pointer_cast<StartMeasurement>(sensor.peripheral);
      if (measure_sensor != nullptr) {
        StartMeasurement::Result result =
            measure_sensor->startMeasurement(JsonVariantConst());
        if (result.error.isError()) {
          TRACEF("Error startMeasurement: %s\r\n",
                 result.error.toString().c_str());
          sensor.state = Sensor::State::kIdle;
          continue;
        }
        sensor.state = Sensor::State::kHandleMeasurement;
        sensor.wait_until = now + result.wait;
        if (result.wait < min_wait) {
          min_wait = result.wait;
        }
      } else {
        sensor.state = Sensor::State::kGetValues;
      }
    }
  }

  for (Sensor& sensor : sensors_) {
    if (sensor.state == Sensor::State::kHandleMeasurement &&
        sensor.wait_until <= now) {
      auto measure_sensor =
          std::dynamic_pointer_cast<StartMeasurement>(sensor.peripheral);
      if (measure_sensor != nullptr) {
        StartMeasurement::Result result = measure_sensor->handleMeasurement();
        if (result.error.isError()) {
          TRACEF("Error handleMeasurement: %s\r\n",
                 result.error.toString().c_str());
          sensor.state = Sensor::State::kFailed;
          continue;
        }
        if (result.wait == std::chrono::nanoseconds::zero()) {
          sensor.state = Sensor::State::kGetValues;
        }
        sensor.wait_until = now + result.wait;
        if (result.wait < min_wait) {
          min_wait = result.wait;
        }
      }
    }

    if (sensor.state == Sensor::State::kGetValues) {
      auto get_values_sensor =
          std::dynamic_pointer_cast<GetValues>(sensor.peripheral);
      if (get_values_sensor != nullptr) {
        GetValues::Result result = get_values_sensor->getValues();
        if (result.error.isError()) {
          TRACEF("Error getValues: %s\r\n", result.error.toString().c_str());
          sensor.state = Sensor::State::kFailed;
          continue;
        }
        for (const utils::ValueUnit& value_unit : result.values) {
          for (DptLimits& dpt_limits : sensor.dpt_limits) {
            if (dpt_limits.dpt != value_unit.data_point_type) {
              continue;
            }
            for (LimitInfo& limit : dpt_limits.limits) {
              handleLimit(value_unit, limit, sensor.peripheral->id, now);
            }
          }
        }
      }
      sensor.state = Sensor::State::kIdle;
    }

    if (sensor.state == Sensor::State::kFailed) {
      sensor.state = Sensor::State::kIdle;
    }
  }

  if (min_wait == std::chrono::nanoseconds::max()) {
    Task::delay(std::chrono::milliseconds(default_interval_).count());
  } else {
    Task::delay(std::chrono::duration_cast<std::chrono::milliseconds>(min_wait)
                    .count());
  }
  return true;
}

void Alarms::handleBehaviorConfig(const JsonObjectConst& config) {
  for (Sensor& sensor : sensors_) {
    for (DptLimits& dpt_limits : sensor.dpt_limits) {
      for (LimitInfo& limit : dpt_limits.limits) {
        setLimitConfig(limit, config[limit.config_key]);
      }
    }
  }
}

void Alarms::setLimitConfig(LimitInfo& limit_info, JsonVariantConst config) {
  if (config.isNull()) {
    return;
  }

  utils::UUID limit_id(config[WebSocket::limit_id_key_]);
  if (limit_id.isValid()) {
    limit_info.limit_id = limit_id;
  }
  JsonVariantConst limit_threshold = config["limit"];
  if (limit_threshold.is<float>()) {
    limit_info.threshold = limit_threshold;
  }
  JsonVariantConst limit_delay_s = config["delay_s"];
  if (limit_delay_s.is<float>()) {
    limit_info.delay_duration = std::chrono::seconds(limit_delay_s);
  }
  JsonVariantConst condition = config["condition"];
  if (condition.is<const char*>()) {
    const String condition_str(condition.as<const char*>());
    if (condition_str == "lt") {
      limit_info.condition = LimitInfo::Condition::kLessThan;
    } else if (condition_str == "gt") {
      limit_info.condition = LimitInfo::Condition::kGreaterThan;
    }
  }
}

void Alarms::handleLimit(const utils::ValueUnit& value_unit,
                         LimitInfo& limit_info,
                         const utils::UUID& peripheral_id,
                         const std::chrono::steady_clock::time_point now) {
  const bool crossed = limit_info.condition == LimitInfo::Condition::kLessThan
                           ? value_unit.value < limit_info.threshold
                           : value_unit.value > limit_info.threshold;

  if (crossed) {
    // Low-pass filter if limit is crossed for longer than delay_duration
    if (ignoreCrossedLimit(limit_info.delay_start, limit_info.delay_duration,
                           now)) {
      return;
    }

    // Notify server that limit has / is being crossed
    if (!limit_info.is_limit_crossed) {
      // Check that this is the first limit crossing
      sendLimitEvent(limit_info, peripheral_id, value_unit,
                     utils::LimitEvent::Type::kStart);
      limit_info.last_continue_event_sent = now;
    } else if (utils::hasTimedOut(limit_info.last_continue_event_sent,
                                  continue_event_period_, now)) {
      // Send continue events periodically while limit is being crossed
      sendLimitEvent(limit_info, peripheral_id, value_unit,
                     utils::LimitEvent::Type::kContinue);
      limit_info.last_continue_event_sent = now;
    }

    limit_info.is_limit_crossed = true;
  } else {
    // If under the threshold, reset the delay start timer
    limit_info.delay_start = std::chrono::steady_clock::time_point::min();

    // First check whether limit is no longer being crossed:
    //   If under the limit and the previous iteration was above the limit and
    //   was activated (limit crossed longer than the delay duration).
    if (limit_info.is_limit_crossed) {
      sendLimitEvent(limit_info, peripheral_id, value_unit,
                     utils::LimitEvent::Type::kEnd);
      limit_info.is_limit_crossed = false;
    }
  }
}

void Alarms::sendLimitEvent(const LimitInfo& limit_info,
                            const utils::UUID& peripheral_id,
                            const utils::ValueUnit& value_unit,
                            const utils::LimitEvent::Type type) {
  // Don't send if the server has not configured this limit
  if (!limit_info.limit_id.isValid()) {
    return;
  }

  JsonDocument limit_event;
  if (Services::is_time_synced_) {
    limit_event[WebSocket::time_key_] = utils::getIsoTimestamp();
  }
  limit_event[WebSocket::limit_id_key_] = limit_info.limit_id.toString();
  limit_event[utils::ValueUnit::value_key] = value_unit.value;
  limit_event[WebSocket::fixed_peripheral_id_key_] = peripheral_id.toString();
  limit_event[WebSocket::fixed_dpt_id_key_] =
      value_unit.data_point_type.toString();

  switch (type) {
    case utils::LimitEvent::Type::kStart:
      limit_event[utils::LimitEvent::event_type_key_] =
          utils::LimitEvent::start_type_;
      break;
    case utils::LimitEvent::Type::kContinue:
      limit_event[utils::LimitEvent::event_type_key_] =
          utils::LimitEvent::continue_type_;
      break;
    case utils::LimitEvent::Type::kEnd:
      limit_event[utils::LimitEvent::event_type_key_] =
          utils::LimitEvent::end_type_;
      break;
  }

  web_socket_->sendLimitEvent(limit_event.as<JsonObject>());
}

bool Alarms::ignoreCrossedLimit(
    std::chrono::steady_clock::time_point& start,
    const std::chrono::milliseconds duration,
    const std::chrono::steady_clock::time_point now) {
  if (duration > std::chrono::seconds::zero()) {
    if (start == std::chrono::steady_clock::time_point::min()) {
      // Delay / filter just started
      start = now;
      return true;
    } else if (!utils::hasTimedOut(start, duration, now)) {
      // Delay / filter waiting for limit_delay_duration to be passed
      return true;
    }
  }
  return false;
}

const std::chrono::milliseconds Alarms::default_interval_{1000};

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata

#endif
