#ifdef DEVICE_TYPE_FIRE_DATA_LOGGER

#include "alarms.h"

#include "managers/time_manager.h"
#include "peripheral/fixed.h"
#include "utils/chrono.h"
#include "utils/range.h"

namespace inamata {
namespace tasks {
namespace fixed {

using namespace std::placeholders;

Alarms::Alarms(const ServiceGetters& services, Scheduler& scheduler,
               const JsonObjectConst& behavior_config)
    : BaseTask(scheduler, Input(nullptr, true)),
      gsm_network_(services.getGsmNetwork()),
      config_manager_(services.getConfigManager()),
      web_socket_(services.getWebSocket()) {
  if (!isValid()) {
    return;
  }

  if (web_socket_ == nullptr) {
    setInvalid(services.web_socket_nullptr_error_);
    return;
  }
  if (gsm_network_ == nullptr) {
    setInvalid(services.gsm_network_nullptr_error_);
    return;
  }
  if (config_manager_ == nullptr) {
    setInvalid(services.config_manager_nullptr_error_);
    return;
  }

  auto& peripheral_controller = Services::getPeripheralController();
  input_bank_1_ =
      std::dynamic_pointer_cast<PCA9539>(peripheral_controller.getPeripheral(
          peripheral::fixed::peripheral_io_1_id));
  input_bank_2_ =
      std::dynamic_pointer_cast<PCA9539>(peripheral_controller.getPeripheral(
          peripheral::fixed::peripheral_io_2_id));
  input_bank_4_[0] =
      std::dynamic_pointer_cast<DigitalIn>(peripheral_controller.getPeripheral(
          peripheral::fixed::peripheral_electric_control_circuit_fail_id));
  input_bank_4_[1] =
      std::dynamic_pointer_cast<DigitalIn>(peripheral_controller.getPeripheral(
          peripheral::fixed::peripheral_jockey_1_pump_run_id));
  input_bank_4_[2] =
      std::dynamic_pointer_cast<DigitalIn>(peripheral_controller.getPeripheral(
          peripheral::fixed::peripheral_jockey_2_pump_run_id));
  input_bank_4_[3] =
      std::dynamic_pointer_cast<DigitalIn>(peripheral_controller.getPeripheral(
          peripheral::fixed::peripheral_jockey_1_pump_fail_id));
  input_bank_4_[4] =
      std::dynamic_pointer_cast<DigitalIn>(peripheral_controller.getPeripheral(
          peripheral::fixed::peripheral_jockey_2_pump_fail_id));
  input_bank_4_[5] =
      std::dynamic_pointer_cast<DigitalIn>(peripheral_controller.getPeripheral(
          peripheral::fixed::peripheral_pumphouse_protection_alarm_id));
  input_bank_4_[6] =
      std::dynamic_pointer_cast<DigitalIn>(peripheral_controller.getPeripheral(
          peripheral::fixed::peripheral_annunciator_fault_id));
  input_bank_4_[7] =
      std::dynamic_pointer_cast<DigitalIn>(peripheral_controller.getPeripheral(
          peripheral::fixed::peripheral_pumphouse_flooding_alarm_id));
  maintenance_input_ =
      std::dynamic_pointer_cast<DigitalIn>(peripheral_controller.getPeripheral(
          peripheral::fixed::peripheral_maintenance_input_id));
  maintenance_button_ =
      std::dynamic_pointer_cast<DigitalIn>(peripheral_controller.getPeripheral(
          peripheral::fixed::peripheral_maintenance_button_id));
  status_led_ =
      std::dynamic_pointer_cast<NeoPixel>(peripheral_controller.getPeripheral(
          peripheral::fixed::peripheral_status_led_id));
  relay_1_ =
      std::dynamic_pointer_cast<DigitalOut>(peripheral_controller.getPeripheral(
          peripheral::fixed::peripheral_relay_1_id));
  relay_2_ =
      std::dynamic_pointer_cast<DigitalOut>(peripheral_controller.getPeripheral(
          peripheral::fixed::peripheral_relay_2_id));

  if (!input_bank_1_ || !input_bank_2_ || !input_bank_4_[0] ||
      !input_bank_4_[1] || !input_bank_4_[2] || !input_bank_4_[3] ||
      !input_bank_4_[4] || !input_bank_4_[5] || !input_bank_4_[6] ||
      !input_bank_4_[7] || !maintenance_input_ || !maintenance_button_ ||
      !status_led_ || !relay_1_ || !relay_2_) {
    char buffer[54];
    const int len = snprintf(
        buffer, sizeof(buffer),
        "Missing peri: %d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d",
        bool(input_bank_1_), bool(input_bank_2_), bool(input_bank_4_[0]),
        bool(input_bank_4_[1]), bool(input_bank_4_[2]), bool(input_bank_4_[3]),
        bool(input_bank_4_[4]), bool(input_bank_4_[5]), bool(input_bank_4_[6]),
        bool(input_bank_4_[7]), bool(maintenance_input_),
        bool(maintenance_button_), bool(status_led_), bool(relay_1_),
        bool(relay_2_));
    setInvalid(buffer);
    return;
  }

  resetLimits();

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
  static const String name{"FireAlarm"};
  return name;
}

bool Alarms::TaskCallback() {
  Task::delay(std::chrono::milliseconds(default_interval_).count());
  now_ = std::chrono::steady_clock::now();
  handleMaintenanceMode();
  handleSmsReminders();

  if (!is_maintenance_mode_) {
    auto result = input_bank_1_->getValues();
    for (const auto& value : result.values) {
      handleResult(value);
    }
    result = input_bank_2_->getValues();
    for (const auto& value : result.values) {
      handleResult(value);
    }
    for (const auto& digital_in : input_bank_4_) {
      result = digital_in->getValues();
      if (result.values.size() == 1) {
        handleResult(result.values[0]);
      } else {
        TRACEF("Missing value: %s", result.error.toString().c_str());
      }
    }
  }
  return true;
}

void Alarms::handleBehaviorConfig(const JsonObjectConst& config) {
  // Parse and validate config fields
  for (BoolLimit* bool_limit : bool_limits_) {
    setBoolLimitConfig(bool_limit, config);
  }

  setDurationLimitConfig(&limit_duration_jockey_1_pump_run_, config);
  setDurationLimitConfig(&limit_duration_jockey_2_pump_run_, config);
  setActivationLimitConfig(&limit_activation_jockey_1_pump_run_, config);
  setActivationLimitConfig(&limit_activation_jockey_2_pump_run_, config);
  setMaintenanceLimitConfig(&maintenance_limit_, config);
}

void Alarms::resetLimits() {
  // Digital alarms
  for (BoolLimit* bool_limit : bool_limits_) {
    resetBoolLimit(bool_limit);
  }

  // Runtime alarms
  resetDurationLimit(&limit_duration_jockey_1_pump_run_);
  resetDurationLimit(&limit_duration_jockey_2_pump_run_);
  resetActivationLimit(&limit_activation_jockey_1_pump_run_);
  resetActivationLimit(&limit_activation_jockey_2_pump_run_);

  // Clear all reminders
  reminder_limits_.clear();
}

void Alarms::handleResult(const utils::ValueUnit& value_unit) {
  /// Digital alarms
  if (peripheral::fixed::dpt_diesel_1_fire_alarm_id ==
      value_unit.data_point_type) {
    handleBoolLimit(limit_diesel_1_fire_alarm_, value_unit);
  } else if (peripheral::fixed::dpt_diesel_2_fire_alarm_id ==
             value_unit.data_point_type) {
    handleBoolLimit(limit_diesel_2_fire_alarm_, value_unit);
  } else if (peripheral::fixed::dpt_diesel_3_fire_alarm_id ==
             value_unit.data_point_type) {
    handleBoolLimit(limit_diesel_3_fire_alarm_, value_unit);
  } else if (peripheral::fixed::dpt_diesel_4_fire_alarm_id ==
             value_unit.data_point_type) {
    handleBoolLimit(limit_diesel_4_fire_alarm_, value_unit);
  }

  else if (peripheral::fixed::dpt_diesel_1_pump_fail_id ==
           value_unit.data_point_type) {
    handleBoolLimit(limit_diesel_1_pump_fail_, value_unit);
  } else if (peripheral::fixed::dpt_diesel_2_pump_fail_id ==
             value_unit.data_point_type) {
    handleBoolLimit(limit_diesel_2_pump_fail_, value_unit);
  } else if (peripheral::fixed::dpt_diesel_3_pump_fail_id ==
             value_unit.data_point_type) {
    handleBoolLimit(limit_diesel_3_pump_fail_, value_unit);
  } else if (peripheral::fixed::dpt_diesel_4_pump_fail_id ==
             value_unit.data_point_type) {
    handleBoolLimit(limit_diesel_4_pump_fail_, value_unit);
  }

  else if (peripheral::fixed::dpt_diesel_1_battery_charger_fail_id ==
           value_unit.data_point_type) {
    handleBoolLimit(limit_diesel_1_battery_charger_fail_, value_unit);
  } else if (peripheral::fixed::dpt_diesel_2_battery_charger_fail_id ==
             value_unit.data_point_type) {
    handleBoolLimit(limit_diesel_2_battery_charger_fail_, value_unit);
  } else if (peripheral::fixed::dpt_diesel_3_battery_charger_fail_id ==
             value_unit.data_point_type) {
    handleBoolLimit(limit_diesel_3_battery_charger_fail_, value_unit);
  } else if (peripheral::fixed::dpt_diesel_4_battery_charger_fail_id ==
             value_unit.data_point_type) {
    handleBoolLimit(limit_diesel_4_battery_charger_fail_, value_unit);
  }

  else if (peripheral::fixed::dpt_diesel_1_low_oil_level_fail_id ==
           value_unit.data_point_type) {
    handleBoolLimit(limit_diesel_1_low_oil_level_fail_, value_unit);
  } else if (peripheral::fixed::dpt_diesel_2_low_oil_level_fail_id ==
             value_unit.data_point_type) {
    handleBoolLimit(limit_diesel_2_low_oil_level_fail_, value_unit);
  } else if (peripheral::fixed::dpt_diesel_3_low_oil_level_fail_id ==
             value_unit.data_point_type) {
    handleBoolLimit(limit_diesel_3_low_oil_level_fail_, value_unit);
  } else if (peripheral::fixed::dpt_diesel_4_low_oil_level_fail_id ==
             value_unit.data_point_type) {
    handleBoolLimit(limit_diesel_4_low_oil_level_fail_, value_unit);
  }

  else if (peripheral::fixed::dpt_diesel_control_circuit_fail_id ==
           value_unit.data_point_type) {
    handleBoolLimit(limit_diesel_control_circuit_fail_, value_unit);
  } else if (peripheral::fixed::dpt_diesel_mains_fail_id ==
             value_unit.data_point_type) {
    handleBoolLimit(limit_diesel_mains_fail_, value_unit);
  } else if (peripheral::fixed::dpt_diesel_pump_fail_id ==
             value_unit.data_point_type) {
    handleBoolLimit(limit_diesel_pump_fail_, value_unit);
  } else if (peripheral::fixed::dpt_diesel_engine_overheat_fail_id ==
             value_unit.data_point_type) {
    handleBoolLimit(limit_diesel_engine_overheat_fail_, value_unit);
  } else if (peripheral::fixed::dpt_diesel_fuel_tank_low_id ==
             value_unit.data_point_type) {
    handleBoolLimit(limit_diesel_fuel_tank_low_, value_unit);
  }

  else if (peripheral::fixed::dpt_electric_1_fire_alarm_id ==
           value_unit.data_point_type) {
    handleBoolLimit(limit_electric_1_fire_alarm_, value_unit);
  } else if (peripheral::fixed::dpt_electric_2_fire_alarm_id ==
             value_unit.data_point_type) {
    handleBoolLimit(limit_electric_2_fire_alarm_, value_unit);
  } else if (peripheral::fixed::dpt_electric_1_pump_fail_id ==
             value_unit.data_point_type) {
    handleBoolLimit(limit_electric_1_pump_fail_, value_unit);
  } else if (peripheral::fixed::dpt_electric_2_pump_fail_id ==
             value_unit.data_point_type) {
    handleBoolLimit(limit_electric_2_pump_fail_, value_unit);
  } else if (peripheral::fixed::dpt_electric_mains_fail_id ==
             value_unit.data_point_type) {
    handleBoolLimit(limit_electric_mains_fail_, value_unit);
  } else if (peripheral::fixed::dpt_electric_control_circuit_fail_id ==
             value_unit.data_point_type) {
    handleBoolLimit(limit_electric_control_circuit_fail_, value_unit);
  }

  else if (peripheral::fixed::dpt_jockey_1_pump_fail_id ==
           value_unit.data_point_type) {
    handleBoolLimit(limit_jockey_1_pump_fail_, value_unit);
  } else if (peripheral::fixed::dpt_jockey_2_pump_fail_id ==
             value_unit.data_point_type) {
    handleBoolLimit(limit_jockey_2_pump_fail_, value_unit);
  } else if (peripheral::fixed::dpt_pumphouse_protection_alarm_id ==
             value_unit.data_point_type) {
    handleBoolLimit(limit_pumphouse_protection_alarm_, value_unit);
  } else if (peripheral::fixed::dpt_annunciator_fault_id ==
             value_unit.data_point_type) {
    handleBoolLimit(limit_annunciator_fault_, value_unit);
  } else if (peripheral::fixed::dpt_pumphouse_flooding_alarm_id ==
             value_unit.data_point_type) {
    handleBoolLimit(limit_pumphouse_flooding_alarm_, value_unit);
  }

  /// Runtime alarms
  else if (peripheral::fixed::dpt_jockey_1_pump_run_id ==
           value_unit.data_point_type) {
    handleDurationLimit(limit_duration_jockey_1_pump_run_, value_unit);
    handleActivationLimit(limit_activation_jockey_1_pump_run_, value_unit);
  } else if (peripheral::fixed::dpt_jockey_2_pump_run_id ==
             value_unit.data_point_type) {
    handleDurationLimit(limit_duration_jockey_2_pump_run_, value_unit);
    handleActivationLimit(limit_activation_jockey_2_pump_run_, value_unit);
  }
}

void Alarms::handleBoolLimit(BoolLimit& limit_info,
                             const utils::ValueUnit& value_unit) {
  if (value_unit.value > 0.5) {
    // Low-pass filter if limit is crossed for longer than delay_duration
    if (ignoreCrossedLimit(limit_info.delay_start, limit_info.delay_duration)) {
      return;
    }

    // Notify server that limit has / is being crossed
    if (!limit_info.is_high) {
      // Check that this is the first limit crossing
      sendLimitEvent(&limit_info, value_unit, utils::LimitEvent::Type::kStart);
      limit_info.last_continue_event_sent = now_;
    } else if (now_ - limit_info.last_continue_event_sent >
               continue_event_period_) {
      // Send continue events periodically while limit is being crossed
      sendLimitEvent(&limit_info, value_unit,
                     utils::LimitEvent::Type::kContinue);
      limit_info.last_continue_event_sent = now_;
    }

    limit_info.is_high = true;
  } else {
    // If under the threshold, reset the delay start timer
    limit_info.delay_start = std::chrono::steady_clock::time_point::min();

    // First check whether limit is no longer being crossed:
    //   If under the limit and the previous iteration was above the limit and
    //   was activated (limit crossed longer than the delay duration).
    if (limit_info.is_high) {
      sendLimitEvent(&limit_info, value_unit, utils::LimitEvent::Type::kEnd);
      limit_info.is_high = false;
    }
  }
}

void Alarms::handleDurationLimit(DurationLimit& limit_info,
                                 const utils::ValueUnit& value_unit) {
  if (value_unit.value > 0.5) {
    // Update continuous high start data. Mark start of continuous high period
    if (limit_info.high_start == std::chrono::steady_clock::time_point::min()) {
      limit_info.high_start = now_;
    } else {
      // Check if the continuous high period was exceeded
      if (now_ - limit_info.high_start > limit_info.high_duration) {
        // Send start if no continue event sent yet
        if (limit_info.last_continue_event_sent ==
            std::chrono::steady_clock::time_point::min()) {
          sendLimitEvent(&limit_info, value_unit,
                         utils::LimitEvent::Type::kStart);
          limit_info.last_continue_event_sent = now_;
        } else if (now_ - limit_info.last_continue_event_sent >
                   continue_event_period_) {
          // Already sent start event, check if to send continue again
          sendLimitEvent(&limit_info, value_unit,
                         utils::LimitEvent::Type::kContinue);
          limit_info.last_continue_event_sent = now_;
        }
      }
    }
    limit_info.is_high = true;
  } else {
    // Just transitioned to off/low
    if (limit_info.is_high) {
      // If a start limit event was sent, send an end event
      if (limit_info.last_continue_event_sent !=
          std::chrono::steady_clock::time_point::min()) {
        sendLimitEvent(&limit_info, value_unit, utils::LimitEvent::Type::kEnd);
      }
      limit_info.high_start = std::chrono::steady_clock::time_point::min();
      limit_info.is_high = false;
      limit_info.last_continue_event_sent =
          std::chrono::steady_clock::time_point::min();
    }
  }
}

void Alarms::handleActivationLimit(ActivationLimit& limit_info,
                                   const utils::ValueUnit& value_unit) {
  // Count each high event
  if (value_unit.value > 0.5) {
    // Just transitioned to on/high
    if (!limit_info.is_high) {
      // Increment the highs counter for this period
      limit_info.highs++;
      limit_info.is_high = true;
      if (limit_info.highs > limit_info.highs_per_period) {
        limit_info.is_over_limit = true;
      }
    }
  } else {
    limit_info.is_high = false;
  }

  // If over limit, check if start event was sent or need to send continue
  // event
  if (limit_info.is_over_limit) {
    if (limit_info.last_continue_event_sent ==
        std::chrono::steady_clock::time_point::min()) {
      sendLimitEvent(&limit_info, value_unit, utils::LimitEvent::Type::kStart);
      limit_info.last_continue_event_sent = now_;
    } else if (now_ - limit_info.last_continue_event_sent >
               continue_event_period_) {
      // Already sent start event, check if need to send continue again
      sendLimitEvent(&limit_info, value_unit,
                     utils::LimitEvent::Type::kContinue);
      limit_info.last_continue_event_sent = now_;
    }
  }

  // At the end of the period, reset the counter
  if (now_ - limit_info.period_start > limit_info.period) {
    // Only reset over limit if under the limit for a full period
    if (limit_info.is_over_limit &&
        limit_info.highs < limit_info.highs_per_period) {
      limit_info.is_over_limit = false;
      limit_info.last_continue_event_sent =
          std::chrono::steady_clock::time_point::min();
      sendLimitEvent(&limit_info, value_unit, utils::LimitEvent::Type::kEnd);
    }
    limit_info.period_start = now_;
    limit_info.highs = 0;
  }
}

void Alarms::sendLimitEvent(BaseLimit* limit,
                            const utils::ValueUnit& value_unit,
                            const utils::LimitEvent::Type type) {
  if (limit == nullptr) {
    TRACELN("Limit nullptr");
    return;
  }
  sendStartStopSms(limit, value_unit, type);

  // Don't send if the controller limit ID or FP ID was not set
  if (!limit->limit_id.isValid() || limit->fixed_peripheral_id == nullptr) {
    TRACEF("Limit event: %d : %s\r\n", int(type),
           value_unit.data_point_type.toString().c_str());
    return;
  }

  JsonDocument limit_event;
  if (Services::is_time_synced_) {
    limit_event[WebSocket::time_key_] = utils::getIsoTimestamp();
  }
  limit_event[WebSocket::limit_id_key_] = limit->limit_id.toString();
  limit_event[utils::ValueUnit::value_key] = value_unit.value;
  limit_event[WebSocket::fixed_peripheral_id_key_] =
      limit->fixed_peripheral_id->toString();
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

void Alarms::sendStartStopSms(BaseLimit* limit,
                              const utils::ValueUnit& value_unit,
                              const utils::LimitEvent::Type type) {
  // Check if in GSM mode, skip continue events, check nullptr and skip
  // maintenance mode limit
  if (!gsm_network_->isEnabled() ||
      type == utils::LimitEvent::Type::kContinue || !limit ||
      limit == &maintenance_limit_) {
    TRACEF("GSM: %d, type: %d, maint: %d\r\n", gsm_network_->isEnabled(), type,
           limit == &maintenance_limit_);
    return;
  }

  // For fire alarms, send to both groups, otherwise only to maintenance
  const String text = generateSmsAlertText(limit, type);
  if (limit == &limit_diesel_1_fire_alarm_ ||
      limit == &limit_diesel_2_fire_alarm_ ||
      limit == &limit_diesel_3_fire_alarm_ ||
      limit == &limit_diesel_4_fire_alarm_) {
    for (const Person& contact : config_manager_->getAllContacts()) {
      if (contact.group_data[kGroupDataMaintenanceBit] ||
          contact.group_data[kGroupDataManagementBit]) {
        Serial.println(contact.cleanPhoneNumber());
        Serial.println(text);
        gsm_network_->modem_.sendSMS(contact.cleanPhoneNumber(), text);
      }
    }
  } else {
    for (const Person& contact : config_manager_->getAllContacts()) {
      if (contact.group_data[kGroupDataMaintenanceBit]) {
        Serial.println(contact.cleanPhoneNumber());
        Serial.println(text);
        gsm_network_->modem_.sendSMS(contact.cleanPhoneNumber(), text);
      }
    }
  }

  // Set/reset the start time and SMS reminder state for start and end events
  if (type == utils::LimitEvent::Type::kStart) {
    limit->start_time = now_;
    limit->sms_reminder = SmsReminder::kInitialMessage;
    reminder_limits_.push_back(limit);
  } else if (type == utils::LimitEvent::Type::kEnd) {
    limit->start_time = std::chrono::steady_clock::time_point::min();
    limit->sms_reminder = SmsReminder::kNone;
    // Iterate through all items to ensure if a limit was added twice that no
    // memory leak occurs
    for (auto it = reminder_limits_.begin(); it != reminder_limits_.end();) {
      if (*it == limit) {
        // erase returns the next valid iterator
        it = reminder_limits_.erase(it);
      } else {
        ++it;
      }
    }
  }
}

void Alarms::handleSmsReminders() {
  if (reminder_limits_.size() == 0 || !gsm_network_->isEnabled()) {
    return;
  }

  for (BaseLimit* limit : reminder_limits_) {
    // Check if reminder 1 should be sent. Requires the limit's initial SMS to
    // have been sent and the delay to have elapsed.
    if (limit->sms_reminder == SmsReminder::kInitialMessage) {
      // Check if reminder 1 should be sent yet
      if (utils::chrono_abs(now_ - limit->start_time) > reminder_1_delay_) {
        limit->sms_reminder = SmsReminder::kReminder1;
        const String text = generateSmsReminderText(limit);
        for (const Person& contact : config_manager_->getAllContacts()) {
          if (contact.group_data[kGroupDataMaintenanceBit] ||
              contact.group_data[kGroupDataManagementBit]) {
            Serial.println(contact.cleanPhoneNumber());
            Serial.println(text);
            gsm_network_->modem_.sendSMS(contact.cleanPhoneNumber(), text);
          }
        }
      }
    } else if (limit->sms_reminder == SmsReminder::kReminder1) {
      // Check if reminder 2 should be sent yet
      if (utils::chrono_abs(now_ - limit->start_time) > reminder_2_delay_) {
        limit->sms_reminder = SmsReminder::kReminder2;
        const String text = generateSmsReminderText(limit);
        for (const Person& contact : config_manager_->getAllContacts()) {
          if (contact.group_data[kGroupDataMaintenanceBit] ||
              contact.group_data[kGroupDataManagementBit]) {
            Serial.println(contact.cleanPhoneNumber());
            Serial.println(text);
            gsm_network_->modem_.sendSMS(contact.cleanPhoneNumber(), text);
          }
        }
      }
    }
  }
}

String Alarms::generateSmsAlertText(const BaseLimit* limit,
                                    const utils::LimitEvent::Type type) {
  const char* delimiter = "\r\n";
  String text("Alarm ");
  if (type == utils::LimitEvent::Type::kStart) {
    text += "Start";
  } else if (type == utils::LimitEvent::Type::kEnd) {
    text += "End";
  }
  text += delimiter;
  text += TimeManager::getFormattedTime();
  text += delimiter;
  text += limit->limit_name;
  text += delimiter;
  text += config_manager_->getLocation();
  return GsmNetwork::encodeSms(text.c_str());
}

String Alarms::generateSmsReminderText(const BaseLimit* limit) {
  const char* delimiter = "\r\n";
  String text("Reminder ");
  if (limit->sms_reminder == SmsReminder::kReminder1) {
    text += "1";
  } else if (limit->sms_reminder == SmsReminder::kReminder2) {
    text += "2";
  }
  text += delimiter;
  text += TimeManager::getFormattedTime();
  text += delimiter;
  text += limit->limit_name;
  text += delimiter;
  text += config_manager_->getLocation();
  text += delimiter;
  if (limit->sms_reminder == SmsReminder::kReminder1) {
    text += "1Hr ago";
  } else if (limit->sms_reminder == SmsReminder::kReminder2) {
    text += "2Hrs ago";
  }
  return GsmNetwork::encodeSms(text.c_str());
}

void Alarms::sendMaintenanceDataPoint(bool on) {
  JsonDocument doc_out;
  JsonObject result_object = doc_out.to<JsonObject>();
  WebSocket::packageTelemetry(
      {utils::ValueUnit(on, peripheral::fixed::dpt_maintenance_mode_id)},
      peripheral::fixed::peripheral_maintenance_button_id, true, result_object);
  web_socket_->sendTelemetry(result_object);
}

bool Alarms::ignoreCrossedLimit(std::chrono::steady_clock::time_point& start,
                                const std::chrono::milliseconds duration) {
  if (duration > std::chrono::seconds::zero()) {
    if (start == std::chrono::steady_clock::time_point::min()) {
      // Delay / filter just started
      start = now_;
      return true;
    } else if (start + duration > now_) {
      // Delay / filter waiting for limit_delay_duration to be passed
      return true;
    }
  }
  return false;
}

void Alarms::handleMaintenanceMode() {
  // Set state of on-board maintenance button
  maintenance_button_->update();

  // Set state of external maintenance input button
  maintenance_input_->update();

  if (maintenance_button_->rose() || maintenance_input_->rose()) {
    is_maintenance_mode_ = !is_maintenance_mode_;
    const utils::UUID relay_dpt = utils::UUID(peripheral::fixed::dpt_relay_id);
    if (is_maintenance_mode_) {
      // Entered maintenance mode
      status_led_->setOverride(utils::Color::fromRgbw(100, 0, 0, 0));
      relay_1_->setValue(utils::ValueUnit(1, relay_dpt));
      sendMaintenanceDataPoint(true);
      sendLimitEvent(
          &maintenance_limit_,
          utils::ValueUnit(1, peripheral::fixed::dpt_maintenance_mode_id),
          utils::LimitEvent::Type::kStart);
      maintenance_limit_.last_continue_event_sent = now_;
      // Reset limits to ensure a clean state when leaving mode
      resetLimits();
    } else {
      // Left maintenace mode
      status_led_->clearOverride();
      relay_1_->setValue(utils::ValueUnit(0, relay_dpt));
      sendMaintenanceDataPoint(false);
      sendLimitEvent(
          &maintenance_limit_,
          utils::ValueUnit(1, peripheral::fixed::dpt_maintenance_mode_id),
          utils::LimitEvent::Type::kEnd);
      maintenance_limit_.last_continue_event_sent =
          std::chrono::steady_clock::time_point::min();
    }
  }
  if (maintenance_limit_.last_continue_event_sent !=
          std::chrono::steady_clock::time_point::min() &&
      now_ - maintenance_limit_.last_continue_event_sent >
          continue_event_period_) {
    sendLimitEvent(
        &maintenance_limit_,
        utils::ValueUnit(1, peripheral::fixed::dpt_maintenance_mode_id),
        utils::LimitEvent::Type::kContinue);
    maintenance_limit_.last_continue_event_sent = now_;
  }
}

void Alarms::setBoolLimitConfig(BoolLimit* limit, JsonObjectConst config) {
  if (limit == nullptr) {
    TRACELN("nullptr");
    return;
  }

  JsonObjectConst limit_config = config[limit->limit_name];
  if (!limit_config.isNull()) {
    utils::UUID limit_id(limit_config[WebSocket::limit_id_key_]);
    if (limit_id.isValid()) {
      limit->limit_id = limit_id;
    }
    JsonVariantConst limit_delay_s = limit_config["delay_s"];
    if (limit_delay_s.is<float>()) {
      float limit_delay_ms =
          utils::clampf(limit_delay_s.as<float>(), 0, 1) * 1000;
      limit->delay_duration = std::chrono::milliseconds(int(limit_delay_ms));
    }
  }
}

void Alarms::setDurationLimitConfig(DurationLimit* limit,
                                    JsonObjectConst config) {
  if (limit == nullptr) {
    TRACELN("nullptr");
    return;
  }

  JsonObjectConst limit_config = config[limit->limit_name];
  if (!limit_config.isNull()) {
    utils::UUID limit_id(limit_config[WebSocket::limit_id_key_]);
    if (limit_id.isValid()) {
      limit->limit_id = limit_id;
    }
    JsonVariantConst limit_high_duration_s = limit_config["duration_s"];
    if (limit_high_duration_s.is<float>()) {
      limit->high_duration = std::chrono::seconds(limit_high_duration_s);
    }
  }
}

void Alarms::setActivationLimitConfig(ActivationLimit* limit,
                                      JsonObjectConst config) {
  if (limit == nullptr) {
    TRACELN("nullptr");
    return;
  }

  JsonObjectConst limit_config = config[limit->limit_name];
  if (!limit_config.isNull()) {
    utils::UUID limit_id(limit_config[WebSocket::limit_id_key_]);
    if (limit_id.isValid()) {
      limit->limit_id = limit_id;
    }
    JsonVariantConst max_jockey_starts_per_h = limit_config["starts_per_h"];
    if (max_jockey_starts_per_h.is<float>()) {
      limit->highs_per_period = max_jockey_starts_per_h.as<uint32_t>();
      limit->period = std::chrono::hours(1);
    }
  }
}

void Alarms::setMaintenanceLimitConfig(BaseLimit* limit,
                                       JsonObjectConst config) {
  if (limit == nullptr) {
    TRACELN("nullptr");
    return;
  }

  JsonObjectConst maintenance_limit = config["maintenance_mode"];
  if (!maintenance_limit.isNull()) {
    utils::UUID limit_id(maintenance_limit[WebSocket::limit_id_key_]);
    if (limit_id.isValid()) {
      limit->limit_id = limit_id;
      limit->fixed_peripheral_id =
          &peripheral::fixed::peripheral_maintenance_button_id;
    }
  }
}

void Alarms::resetBoolLimit(BoolLimit* limit) {
  if (limit == nullptr) {
    TRACELN("Nullptr");
    return;
  }

  limit->is_high = false;
  limit->sms_reminder = SmsReminder::kNone;
  limit->last_continue_event_sent =
      std::chrono::steady_clock::time_point::min();
  limit->delay_start = std::chrono::steady_clock::time_point::min();
}

void Alarms::resetDurationLimit(DurationLimit* limit) {
  if (limit == nullptr) {
    TRACELN("Nullptr");
    return;
  }

  limit->is_high = false;
  limit->sms_reminder = SmsReminder::kNone;
  limit->last_continue_event_sent =
      std::chrono::steady_clock::time_point::min();
  limit->high_start = std::chrono::steady_clock::time_point::min();
}

void Alarms::resetActivationLimit(ActivationLimit* limit) {
  if (limit == nullptr) {
    TRACELN("Nullptr");
    return;
  }

  limit->is_high = false;
  limit->sms_reminder = SmsReminder::kNone;
  limit->last_continue_event_sent =
      std::chrono::steady_clock::time_point::min();
  limit->highs = 0;
  limit->period_start = std::chrono::steady_clock::time_point::min();
  limit->is_over_limit = false;
}

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata
#endif