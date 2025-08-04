#ifdef DEVICE_TYPE_TIAKI_CO2_MONITOR

#include "alarms.h"

#include "peripheral/fixed.h"
#include "utils/chrono.h"

namespace inamata {
namespace tasks {
namespace fixed {

using namespace std::placeholders;

Alarms::Alarms(const ServiceGetters& services, Scheduler& scheduler,
               const JsonObjectConst& behavior_config)
    : PollAbstract(scheduler),
      led_dpt_(utils::UUID::fromFSH(peripheral::fixed::dpt_led_id)),
      relay_dpt_(utils::UUID::fromFSH(peripheral::fixed::dpt_relay_id)),
      current_dpt_(utils::UUID::fromFSH(peripheral::fixed::dpt_current_a_id)),
      buzzer_dpt_(utils::UUID::fromFSH(peripheral::fixed::dpt_buzzer_id)),
      co2_dpt_(utils::UUID::fromFSH(peripheral::fixed::dpt_co2_ppm_id)),
      voc_dpt_(utils::UUID::fromFSH(peripheral::fixed::dpt_voc_index_id)) {
  if (!isValid()) {
    return;
  }
  String error = checkLeds();
  if (!error.isEmpty()) {
    setInvalid(error);
    return;
  }
  error = checkRelays();
  if (!error.isEmpty()) {
    setInvalid(error);
    return;
  }

  if (!behavior_config.isNull()) {
    handleBehaviorConfig(behavior_config);
  }
  Services::getBehaviorController().registerConfigCallback(
      std::bind(&Alarms::handleBehaviorConfig, this, _1));

  web_socket_ = services.getWebSocket();
  if (web_socket_ == nullptr) {
    setInvalid(services.web_socket_nullptr_error_);
    return;
  }

  measurement_wait_ = std::chrono::seconds(5);

  setIterations(TASK_FOREVER);
  enable();
}

const String& Alarms::getType() const { return type(); }

const String& Alarms::type() {
  static const String name{"Co2Alarm"};
  return name;
}

bool Alarms::TaskCallback() {
  Task::delay(std::chrono::milliseconds(default_interval_).count());
  handlePoll();
  handleAlarms();
  return true;
}

void Alarms::handleBehaviorConfig(const JsonObjectConst& config) {
  // Parse and validate config fields
  setLimitConfig(co2_limit_1_, config["co2_limit_1"]);
  setLimitConfig(co2_limit_2_, config["co2_limit_2"]);
  setLimitConfig(co2_limit_3_, config["co2_limit_3"]);
  setLimitConfig(co2_limit_4_, config["co2_limit_4"]);
  setLimitConfig(voc_limit_1_, config["voc_limit_1"]);
  setLimitConfig(voc_limit_2_, config["voc_limit_2"]);
}

void Alarms::setLimitConfig(LimitInfo& limit_info, JsonObjectConst config) {
  if (!config.isNull()) {
    utils::UUID limit_id(config[WebSocket::limit_id_key_]);
    if (limit_id.isValid()) {
      limit_info.limit_id = limit_id;
    }
    JsonVariantConst co2_limit_threshold = config["limit"];
    if (co2_limit_threshold.is<float>()) {
      limit_info.threshold = co2_limit_threshold;
    }
    JsonVariantConst limit_delay_s = config["delay_s"];
    if (limit_delay_s.is<float>()) {
      limit_info.delay_duration = std::chrono::seconds(limit_delay_s);
    }
  }
}

void Alarms::handleResult(std::vector<utils::ValueUnit>& values) {
  // Extract the CO2 and VOC values. For CO2, if missing or 0 mark failed.
  bool co2_found = false;
  for (const auto& value_unit : values) {
    if (value_unit.data_point_type == co2_dpt_) {
      if (value_unit.value > 0) {
        co2_found = true;
      }
      last_co2_ppm_ = value_unit.value;
    } else if (value_unit.data_point_type == voc_dpt_) {
      last_voc_index_ = value_unit.value;
    }
  }
  setFaultOutput(!co2_found);
}

void Alarms::onError() {
  // 100 should be around 4:10 minutes (Enough time for an OTA update)
  // 5 seconds standard, 2.5 on error retry
  if (errors_since_last_success_ > 10) {
    setFaultOutput(true);
  }
  if (errors_since_last_success_ > 100) {
    Serial.println("Too many errors ModBus errors");
    ESP.restart();
  }
}

void Alarms::handleAlarms() {
  if (std::isnan(last_co2_ppm_)) {
    return;
  }

  const auto now = std::chrono::steady_clock::now();
  utils::ValueUnit co2_value_unit(last_co2_ppm_, co2_dpt_);
  for (LimitInfo* limit :
       {&co2_limit_1_, &co2_limit_2_, &co2_limit_3_, &co2_limit_4_}) {
    handleLimit(co2_value_unit, *limit, now);
  }

  setAlarmOutputs(co2_limit_1_, led_alarm_1_, relay_alarm_1_);
  setAlarmOutputs(co2_limit_2_, led_alarm_2_, relay_alarm_2_);
  setAlarmOutputs(co2_limit_3_, led_alarm_3_, relay_alarm_3_);
  setAlarmOutputs(co2_limit_4_, led_alarm_4_, relay_alarm_4_);

  const float clamped_co2 = std::max(0.f, std::min(last_co2_ppm_, 6000.f));
  // Formula: t = b1 + (s - a1)(b2 - b1) / (a2 - a1)
  // Where s = input, [a1, a2] = input range, [b1, b2] = output range
  // With range-a is the CO2 ppm measurement and range-b the 4-20ma analog out
  const float current_a = 0.004f + clamped_co2 * (0.02f - 0.004f) / 6000.f;
  analog_out_->setValue(utils::ValueUnit(current_a, current_dpt_));
}

void Alarms::handleLimit(const utils::ValueUnit& value_unit,
                         LimitInfo& limit_info,
                         const std::chrono::steady_clock::time_point now) {
  if (value_unit.value > limit_info.threshold) {
    // Low-pass filter if limit is crossed for longer than limit_delay_duration
    if (ignoreCrossedLimit(limit_info.delay_start, limit_info.delay_duration,
                           now)) {
      return;
    }

    // Notify server that limit has / is being crossed
    if (!limit_info.is_limit_crossed) {
      // Check that this is the first limit crossing
      sendLimitEvent(limit_info.limit_id, value_unit,
                     utils::LimitEvent::Type::kStart);
      limit_info.last_continue_event_sent = now;
    } else if (limit_info.last_continue_event_sent + continue_event_period_ <
               now) {
      // Send continue events periodically while limit is being crossed
      sendLimitEvent(limit_info.limit_id, value_unit,
                     utils::LimitEvent::Type::kContinue);
      limit_info.last_continue_event_sent = now;
    }

    limit_info.is_limit_crossed = true;
  } else {
    // If under the threshold, reset the delay start timer
    limit_info.delay_start = std::chrono::steady_clock::time_point::min();

    // First check where limit is no longer being crossed:
    //   If under the limit and the previous iteration was above the limit and
    //   was activated (limit crossed longer than the delay duration).
    if (limit_info.is_limit_crossed) {
      sendLimitEvent(limit_info.limit_id, value_unit,
                     utils::LimitEvent::Type::kEnd);
      limit_info.is_limit_crossed = false;
    }
  }
}

void Alarms::sendLimitEvent(const utils::UUID& limit_id,
                            const utils::ValueUnit& value_unit,
                            const utils::LimitEvent::Type type) {
  // Don't send if the controller limit ID was not set
  if (!limit_id.isValid()) {
    return;
  }

  JsonDocument limit_event;
  if (Services::is_time_synced_) {
    limit_event[WebSocket::time_key_] = utils::getIsoTimestamp();
  }
  limit_event[WebSocket::limit_id_key_] = limit_id.toString();
  limit_event[utils::ValueUnit::value_key] = value_unit.value;
  limit_event[WebSocket::fixed_peripheral_id_key_] =
      modbus_input_->id.toString();
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
    } else if (start + duration > now) {
      // Delay / filter waiting for limit_delay_duration to be passed
      return true;
    }
  }
  return false;
}

void Alarms::setAlarmOutputs(LimitInfo& limit_info,
                             std::shared_ptr<DigitalOut> led,
                             std::shared_ptr<DigitalOut> relay) {
  led->setValue(limit_info.is_limit_crossed ? utils::ValueUnit(1, led_dpt_)
                                            : utils::ValueUnit(0, led_dpt_));
  relay->setValue(limit_info.is_limit_crossed
                      ? utils::ValueUnit(1, relay_dpt_)
                      : utils::ValueUnit(0, relay_dpt_));
}

void Alarms::setFaultOutput(const bool is_fault) {
  led_fault_->setValue(utils::ValueUnit(is_fault, led_dpt_));
  if (is_fault) {
    buzzer_->setValue(utils::ValueUnit(!buzzer_->getState(), buzzer_dpt_));
    analog_out_->setValue(utils::ValueUnit(0, current_dpt_));
    last_co2_ppm_ = NAN;
  } else {
    buzzer_->setValue(utils::ValueUnit(0, buzzer_dpt_));
  }
}

Alarms::LimitInfo Alarms::makeLimitInfo(
    const float threshold, const std::chrono::seconds delay_duration) {
  return LimitInfo{
      .threshold = threshold,
      .is_limit_crossed = false,
      .limit_id = nullptr,
      .delay_start = std::chrono::steady_clock::time_point::min(),
      .last_continue_event_sent = std::chrono::steady_clock::time_point::min(),
      .delay_duration = delay_duration};
}

const std::chrono::milliseconds Alarms::default_interval_{1000};

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata

#endif