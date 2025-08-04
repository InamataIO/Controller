#ifdef DEVICE_TYPE_VOC_SENSOR_MK1

#include "alert.h"

#include "peripheral/fixed.h"
#include "utils/chrono.h"

namespace inamata {
namespace tasks {
namespace fixed {

using namespace std::placeholders;

Alert::Alert(const ServiceGetters& services, Scheduler& scheduler,
             const JsonObjectConst& behavior_config)
    : BaseTask(scheduler, Input(nullptr, true)),
      scheduler_(scheduler),
      services_(services) {
  if (!isValid()) {
    return;
  }

  if (!behavior_config.isNull()) {
    handleBehaviorConfig(behavior_config);
  }
  Services::getBehaviorController().registerConfigCallback(
      std::bind(&Alert::handleBehaviorConfig, this, _1));

  web_socket_ = services.getWebSocket();
  if (web_socket_ == nullptr) {
    setInvalid(services.web_socket_nullptr_error_);
    return;
  }

  bool success = setFixedPeripherals();
  if (!success) {
    return;
  }

  Task::setInterval(std::chrono::milliseconds(default_interval_).count());
  setIterations(TASK_FOREVER);
  enable();
}

const String& Alert::getType() const { return type(); }

const String& Alert::type() {
  static const String name{"Alert"};
  return name;
}

bool Alert::TaskCallback() {
  // Get the values and check for error
  peripheral::capabilities::GetValues::Result result = voc_sensor_->getValues();
  // Try again next iteration on error or missing reading
  if (result.error.isError() || result.values.size() < 1) {
    return true;
  }

  const utils::ValueUnit& value_unit = result.values[0];
  const std::chrono::steady_clock::time_point now =
      std::chrono::steady_clock::now();

  update_color_ = ok_color_;
  handleLimit1(now, value_unit);
  handleLimit2(now, value_unit);
  status_led_->turnOn(update_color_);

  return true;
}

void Alert::handleBehaviorConfig(const JsonObjectConst& config) {
  // Parse and validate config fields
  JsonObjectConst voc_config = config["voc_limit_1"];
  if (!voc_config.isNull()) {
    utils::UUID alert_id(voc_config[limit_id_key_]);
    if (alert_id.isValid()) {
      limit_1_id_ = alert_id;
    }
    JsonVariantConst voc_alert_limit = voc_config["limit"];
    if (voc_alert_limit.is<float>()) {
      voc_alert_limit_1_ = voc_alert_limit;
    }
    JsonVariantConst limit_delay_s = voc_config["delay_s"];
    if (limit_delay_s.is<float>()) {
      limit_1_delay_duration = std::chrono::seconds(limit_delay_s);
    }
  }
  voc_config = config["voc_limit_2"];
  if (!voc_config.isNull()) {
    utils::UUID alert_id(voc_config[limit_id_key_]);
    if (alert_id.isValid()) {
      limit_2_id_ = alert_id;
    }
    JsonVariantConst voc_alert_limit = voc_config["limit"];
    if (voc_alert_limit.is<float>()) {
      voc_alert_limit_2_ = voc_alert_limit;
    }
    JsonVariantConst limit_delay_s = voc_config["delay_s"];
    if (limit_delay_s.is<float>()) {
      limit_2_delay_duration = std::chrono::seconds(limit_delay_s);
    }
  }
  TRACEF("1: id: %s, lim: %f, del: %d\r\n", limit_1_id_.toString().c_str(),
         voc_alert_limit_1_, limit_1_delay_duration.count());
  TRACEF("2: id: %s, lim: %f, del: %d\r\n", limit_2_id_.toString().c_str(),
         voc_alert_limit_2_, limit_2_delay_duration.count());
}

bool Alert::setFixedPeripherals() {
  utils::UUID peripheral_id(
      String(peripheral::fixed::peripheral_voc_id).c_str());
  std::shared_ptr<peripheral::Peripheral> peripheral =
      Services::getPeripheralController().getPeripheral(peripheral_id);
  if (!peripheral) {
    setInvalid(peripheral::Peripheral::peripheralNotFoundError(peripheral_id));
    return false;
  }

  // Since the UUID is specified externally, check the type
  const String& sgp40_type = peripheral::peripherals::sgp40::SGP40::type();
  if (peripheral->getType() == sgp40_type && peripheral->isValid()) {
    voc_sensor_ =
        std::static_pointer_cast<peripheral::peripherals::sgp40::SGP40>(
            peripheral);
  } else {
    setInvalid(
        peripheral::Peripheral::notAValidError(peripheral_id, sgp40_type));
    return false;
  }

  peripheral_id = String(peripheral::fixed::peripheral_status_led_id).c_str();
  peripheral = Services::getPeripheralController().getPeripheral(peripheral_id);
  if (!peripheral) {
    setInvalid(peripheral::Peripheral::peripheralNotFoundError(peripheral_id));
    return false;
  }

  // Since the UUID is specified externally, check the type
  const String& neo_pixel_type =
      peripheral::peripherals::neo_pixel::NeoPixel::type();
  if (peripheral->getType() == neo_pixel_type && peripheral->isValid()) {
    status_led_ =
        std::static_pointer_cast<peripheral::peripherals::neo_pixel::NeoPixel>(
            peripheral);
  } else {
    setInvalid(
        peripheral::Peripheral::notAValidError(peripheral_id, neo_pixel_type));
    return false;
  }
  return true;
}

void Alert::handleLimit1(const std::chrono::steady_clock::time_point now,
                         const utils::ValueUnit& value_unit) {
  if (value_unit.value > voc_alert_limit_1_) {
    // Low-pass filter if limit is crossed for longer than limit_delay_duration
    if (ignoreCrossedLimit(limit_1_delay_start, limit_1_delay_duration, now)) {
      return;
    }

    // Set status LED and notify server that limit has / is being crossed
    if (!is_limit_1_crossed) {
      // Check that this is the first limit crossing
      sendLimitEvent(limit_1_id_, value_unit, utils::LimitEvent::Type::kStart);
      last_continue_event_1_sent = now;
    } else if (last_continue_event_1_sent + continue_event_period_ < now) {
      // Send continue events periodically while limit is being crossed
      sendLimitEvent(limit_1_id_, value_unit,
                     utils::LimitEvent::Type::kContinue);
      last_continue_event_1_sent = now;
    }

    update_color_ = elevated_color_;
    is_limit_1_crossed = true;
  } else if (is_limit_1_crossed) {
    // TODO: Check Alarms::handleLimit to always set limit_2_delay_start to
    // min if below threshold

    // First check where limit is no longer being crossed
    limit_1_delay_start = std::chrono::steady_clock::time_point::min();
    sendLimitEvent(limit_1_id_, value_unit, utils::LimitEvent::Type::kEnd);
    is_limit_1_crossed = false;
  }
}

void Alert::handleLimit2(const std::chrono::steady_clock::time_point now,
                         const utils::ValueUnit& value_unit) {
  if (value_unit.value > voc_alert_limit_2_) {
    // Low-pass filter if limit is crossed for longer than limit_delay_duration
    if (ignoreCrossedLimit(limit_2_delay_start, limit_2_delay_duration, now)) {
      return;
    }

    // Notify server that limit has / is being crossed
    if (!is_limit_2_crossed) {
      // Check that this is the first limit crossing
      sendLimitEvent(limit_2_id_, value_unit, utils::LimitEvent::Type::kStart);
      last_continue_event_2_sent = now;
    } else if (last_continue_event_2_sent + continue_event_period_ < now) {
      // Send continue events periodically while limit is being crossed
      sendLimitEvent(limit_2_id_, value_unit,
                     utils::LimitEvent::Type::kContinue);
      last_continue_event_2_sent = now;
    }

    update_color_ = danger_color_;
    is_limit_2_crossed = true;
  } else if (is_limit_2_crossed) {
    // TODO: Check Alarms::handleLimit to always set limit_2_delay_start to
    // min if below threshold

    // First check where limit is no longer being crossed
    limit_2_delay_start = std::chrono::steady_clock::time_point::min();
    sendLimitEvent(limit_2_id_, value_unit, utils::LimitEvent::Type::kEnd);
    is_limit_2_crossed = false;
  }
}

bool Alert::ignoreCrossedLimit(
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

void Alert::sendLimitEvent(const utils::UUID& limit_id,
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
  limit_event[limit_id_key_] = limit_id.toString();
  limit_event[utils::ValueUnit::value_key] = value_unit.value;
  limit_event[fixed_peripheral_id_key_] = voc_sensor_->id.toString();
  limit_event[fixed_dpt_id_key_] = value_unit.data_point_type.toString();
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

const std::chrono::seconds Alert::default_interval_{1};
const __FlashStringHelper* Alert::limit_id_key_ = FPSTR("limit_id");
const __FlashStringHelper* Alert::fixed_peripheral_id_key_ = FPSTR("fp_id");
const __FlashStringHelper* Alert::fixed_dpt_id_key_ = FPSTR("fdpt_id");

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata

#endif