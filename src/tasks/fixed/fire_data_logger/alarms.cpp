#ifdef DEVICE_TYPE_FIRE_DATA_LOGGER

#include "alarms.h"

#include "peripheral/fixed.h"

namespace inamata {
namespace tasks {
namespace fixed {

Alarms::Alarms(const ServiceGetters& services, Scheduler& scheduler,
               const JsonObjectConst& behavior_config)
    : BaseTask(scheduler, Input(nullptr, true)) {
  if (!isValid()) {
    return;
  }

  auto& peripheral_controller = Services::getPeripheralController();
  input_bank_1_ =
      std::dynamic_pointer_cast<PCA9539>(peripheral_controller.getPeripheral(
          peripheral::fixed::peripheral_io_1_id));
  input_bank_2_ =
      std::dynamic_pointer_cast<PCA9539>(peripheral_controller.getPeripheral(
          peripheral::fixed::peripheral_io_2_id));
  input_bank_3_[0] =
      std::dynamic_pointer_cast<DigitalIn>(peripheral_controller.getPeripheral(
          peripheral::fixed::peripheral_electric_control_circuit_fail_id));
  input_bank_3_[1] =
      std::dynamic_pointer_cast<DigitalIn>(peripheral_controller.getPeripheral(
          peripheral::fixed::peripheral_jockey_1_pump_run_id));
  input_bank_3_[2] =
      std::dynamic_pointer_cast<DigitalIn>(peripheral_controller.getPeripheral(
          peripheral::fixed::peripheral_jockey_2_pump_run_id));
  input_bank_3_[3] =
      std::dynamic_pointer_cast<DigitalIn>(peripheral_controller.getPeripheral(
          peripheral::fixed::peripheral_jockey_1_pump_fail_id));
  input_bank_3_[4] =
      std::dynamic_pointer_cast<DigitalIn>(peripheral_controller.getPeripheral(
          peripheral::fixed::peripheral_jockey_2_pump_fail_id));
  input_bank_3_[5] =
      std::dynamic_pointer_cast<DigitalIn>(peripheral_controller.getPeripheral(
          peripheral::fixed::peripheral_pumphouse_protection_alarm_id));
  input_bank_3_[6] =
      std::dynamic_pointer_cast<DigitalIn>(peripheral_controller.getPeripheral(
          peripheral::fixed::peripheral_annunciator_fault_id));
  input_bank_3_[7] =
      std::dynamic_pointer_cast<DigitalIn>(peripheral_controller.getPeripheral(
          peripheral::fixed::peripheral_pumphouse_flooding_alarm_id));
  maintenance_mode_ =
      std::dynamic_pointer_cast<DigitalIn>(peripheral_controller.getPeripheral(
          peripheral::fixed::peripheral_maintenance_mode_id));

  if (!input_bank_1_ || !input_bank_2_ || !input_bank_3_[0] ||
      !input_bank_3_[1] || !input_bank_3_[2] || !input_bank_3_[3] ||
      !input_bank_3_[4] || !input_bank_3_[5] || !input_bank_3_[6] ||
      !input_bank_3_[7] || !maintenance_mode_) {
    char buffer[40];
    const int len = snprintf(
        buffer, sizeof(buffer),
        "Missing peri: %d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d", bool(input_bank_1_),
        bool(input_bank_2_), bool(input_bank_3_[0]), bool(input_bank_3_[1]),
        bool(input_bank_3_[2]), bool(input_bank_3_[3]), bool(input_bank_3_[4]),
        bool(input_bank_3_[5]), bool(input_bank_3_[6]), bool(input_bank_3_[7]),
        bool(maintenance_mode_));
    setInvalid(buffer);
    return;
  }

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

  auto result = input_bank_1_->getValues();
  for (const auto& value : result.values) {
    handleResult(value);
  }
  result = input_bank_2_->getValues();
  for (const auto& value : result.values) {
    handleResult(value);
  }
  for (const auto& digital_in : input_bank_3_) {
    result = digital_in->getValues();
    if (result.values.size() == 1) {
      handleResult(result.values[0]);
    } else {
      TRACEF("Missing value: %s", result.error.toString().c_str());
    }
  }
  return true;
}

void Alarms::handleResult(const utils::ValueUnit& value_unit) {
  const auto now = std::chrono::steady_clock::now();
  /// Digital alarms
  if (peripheral::fixed::dpt_diesel_1_fire_alarm_id ==
      value_unit.data_point_type) {
    handleBoolLimit(limit_diesel_1_fire_alarm_, value_unit, now);
  } else if (peripheral::fixed::dpt_diesel_2_fire_alarm_id ==
             value_unit.data_point_type) {
    handleBoolLimit(limit_diesel_2_fire_alarm_, value_unit, now);
  } else if (peripheral::fixed::dpt_diesel_3_fire_alarm_id ==
             value_unit.data_point_type) {
    handleBoolLimit(limit_diesel_3_fire_alarm_, value_unit, now);
  } else if (peripheral::fixed::dpt_diesel_4_fire_alarm_id ==
             value_unit.data_point_type) {
    handleBoolLimit(limit_diesel_4_fire_alarm_, value_unit, now);
  }

  else if (peripheral::fixed::dpt_diesel_1_pump_fail_id ==
           value_unit.data_point_type) {
    handleBoolLimit(limit_diesel_1_pump_fail_, value_unit, now);
  } else if (peripheral::fixed::dpt_diesel_2_pump_fail_id ==
             value_unit.data_point_type) {
    handleBoolLimit(limit_diesel_2_pump_fail_, value_unit, now);
  } else if (peripheral::fixed::dpt_diesel_3_pump_fail_id ==
             value_unit.data_point_type) {
    handleBoolLimit(limit_diesel_3_pump_fail_, value_unit, now);
  } else if (peripheral::fixed::dpt_diesel_4_pump_fail_id ==
             value_unit.data_point_type) {
    handleBoolLimit(limit_diesel_4_pump_fail_, value_unit, now);
  }

  else if (peripheral::fixed::dpt_diesel_1_battery_charger_fail_id ==
           value_unit.data_point_type) {
    handleBoolLimit(limit_diesel_1_battery_charger_fail_, value_unit, now);
  } else if (peripheral::fixed::dpt_diesel_2_battery_charger_fail_id ==
             value_unit.data_point_type) {
    handleBoolLimit(limit_diesel_2_battery_charger_fail_, value_unit, now);
  } else if (peripheral::fixed::dpt_diesel_3_battery_charger_fail_id ==
             value_unit.data_point_type) {
    handleBoolLimit(limit_diesel_3_battery_charger_fail_, value_unit, now);
  } else if (peripheral::fixed::dpt_diesel_4_battery_charger_fail_id ==
             value_unit.data_point_type) {
    handleBoolLimit(limit_diesel_4_battery_charger_fail_, value_unit, now);
  }

  else if (peripheral::fixed::dpt_diesel_1_low_oil_level_fail_id ==
           value_unit.data_point_type) {
    handleBoolLimit(limit_diesel_1_low_oil_level_fail_, value_unit, now);
  } else if (peripheral::fixed::dpt_diesel_2_low_oil_level_fail_id ==
             value_unit.data_point_type) {
    handleBoolLimit(limit_diesel_2_low_oil_level_fail_, value_unit, now);
  } else if (peripheral::fixed::dpt_diesel_3_low_oil_level_fail_id ==
             value_unit.data_point_type) {
    handleBoolLimit(limit_diesel_3_low_oil_level_fail_, value_unit, now);
  } else if (peripheral::fixed::dpt_diesel_4_low_oil_level_fail_id ==
             value_unit.data_point_type) {
    handleBoolLimit(limit_diesel_4_low_oil_level_fail_, value_unit, now);
  }

  else if (peripheral::fixed::dpt_diesel_control_circuit_fail_id ==
           value_unit.data_point_type) {
    handleBoolLimit(limit_diesel_control_circuit_fail_, value_unit, now);
  } else if (peripheral::fixed::dpt_diesel_mains_fail_id ==
             value_unit.data_point_type) {
    handleBoolLimit(limit_diesel_mains_fail_, value_unit, now);
  } else if (peripheral::fixed::dpt_diesel_pump_fail_id ==
             value_unit.data_point_type) {
    handleBoolLimit(limit_diesel_pump_fail_, value_unit, now);
  } else if (peripheral::fixed::dpt_diesel_engine_overheat_fail_id ==
             value_unit.data_point_type) {
    handleBoolLimit(limit_diesel_engine_overheat_fail_, value_unit, now);
  } else if (peripheral::fixed::dpt_diesel_fuel_tank_low_id ==
             value_unit.data_point_type) {
    handleBoolLimit(limit_diesel_fuel_tank_low_, value_unit, now);
  }

  else if (peripheral::fixed::dpt_electric_1_fire_alarm_id ==
           value_unit.data_point_type) {
    handleBoolLimit(limit_electric_1_fire_alarm_, value_unit, now);
  } else if (peripheral::fixed::dpt_electric_2_fire_alarm_id ==
             value_unit.data_point_type) {
    handleBoolLimit(limit_electric_2_fire_alarm_, value_unit, now);
  } else if (peripheral::fixed::dpt_electric_1_pump_fail_id ==
             value_unit.data_point_type) {
    handleBoolLimit(limit_electric_1_pump_fail_, value_unit, now);
  } else if (peripheral::fixed::dpt_electric_2_pump_fail_id ==
             value_unit.data_point_type) {
    handleBoolLimit(limit_electric_2_pump_fail_, value_unit, now);
  } else if (peripheral::fixed::dpt_electric_mains_fail_id ==
             value_unit.data_point_type) {
    handleBoolLimit(limit_electric_mains_fail_, value_unit, now);
  } else if (peripheral::fixed::dpt_electric_control_circuit_fail_id ==
             value_unit.data_point_type) {
    handleBoolLimit(limit_electric_control_circuit_fail_, value_unit, now);
  }

  else if (peripheral::fixed::dpt_jockey_1_pump_fail_id ==
           value_unit.data_point_type) {
    handleBoolLimit(limit_jockey_1_pump_fail_, value_unit, now);
  } else if (peripheral::fixed::dpt_jockey_2_pump_fail_id ==
             value_unit.data_point_type) {
    handleBoolLimit(limit_jockey_2_pump_fail_, value_unit, now);
  }

  /// Runtime alarms
  else if (value_unit.data_point_type ==
           peripheral::fixed::dpt_jockey_1_pump_run_id) {
    handleDurationLimit(limit_duration_jockey_1_pump_run_, value_unit, now);
    handleActivationLimit(limit_activation_jockey_1_pump_run_, value_unit, now);
  } else if (value_unit.data_point_type ==
             peripheral::fixed::dpt_jockey_2_pump_run_id) {
    handleDurationLimit(limit_duration_jockey_2_pump_run_, value_unit, now);
    handleActivationLimit(limit_activation_jockey_2_pump_run_, value_unit, now);
  }
}

void Alarms::handleBoolLimit(BoolLimit& limit_info,
                             const utils::ValueUnit& value_unit,
                             const std::chrono::steady_clock::time_point now) {
  if (value_unit.value > 0.5) {
    // Low-pass filter if limit is crossed for longer than limit_delay_duration
    if (ignoreCrossedLimit(limit_info.delay_start, limit_info.delay_duration,
                           now)) {
      return;
    }

    // Notify server that limit has / is being crossed
    if (!limit_info.is_high) {
      // Check that this is the first limit crossing
      sendLimitEvent(limit_info.limit_id, limit_info.fixed_peripheral_id,
                     value_unit, utils::LimitEvent::Type::kStart);
      limit_info.last_continue_event_sent = now;
    } else if (limit_info.last_continue_event_sent + continue_event_period_ <
               now) {
      // Send continue events periodically while limit is being crossed
      sendLimitEvent(limit_info.limit_id, limit_info.fixed_peripheral_id,
                     value_unit, utils::LimitEvent::Type::kContinue);
      limit_info.last_continue_event_sent = now;
    }

    limit_info.is_high = true;
  } else {
    // If under the threshold, reset the delay start timer
    limit_info.delay_start = std::chrono::steady_clock::time_point::min();

    // First check whether limit is no longer being crossed:
    //   If under the limit and the previous iteration was above the limit and
    //   was activated (limit crossed longer than the delay duration).
    if (limit_info.is_high) {
      sendLimitEvent(limit_info.limit_id, limit_info.fixed_peripheral_id,
                     value_unit, utils::LimitEvent::Type::kEnd);
      limit_info.is_high = false;
    }
  }
}

void Alarms::handleDurationLimit(
    DurationLimit& limit_info, const utils::ValueUnit& value_unit,
    const std::chrono::steady_clock::time_point now) {
  if (value_unit.value > 0.5) {
    // Update continuous high start data
    // Mark start of continuous high period
    if (limit_info.high_start == std::chrono::steady_clock::time_point::min()) {
      limit_info.high_start = std::chrono::steady_clock::now();
    } else {
      // Check if the continuous high period was exceeded
      if (now - limit_info.high_start > limit_info.high_duration) {
        // Send start if no continue event sent yet
        if (limit_info.last_continue_event_sent ==
            std::chrono::steady_clock::time_point::min()) {
          sendLimitEvent(limit_info.limit_id, limit_info.fixed_peripheral_id,
                         value_unit, utils::LimitEvent::Type::kStart);
          limit_info.last_continue_event_sent = now;
        } else if (now - limit_info.last_continue_event_sent >
                   continue_event_period_) {
          // Already sent start event, check if to send continue again
          sendLimitEvent(limit_info.limit_id, limit_info.fixed_peripheral_id,
                         value_unit, utils::LimitEvent::Type::kContinue);
          limit_info.last_continue_event_sent = now;
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
        sendLimitEvent(limit_info.limit_id, limit_info.fixed_peripheral_id,
                       value_unit, utils::LimitEvent::Type::kEnd);
      }
      limit_info.high_start = std::chrono::steady_clock::time_point::min();
      limit_info.is_high = false;
      limit_info.last_continue_event_sent =
          std::chrono::steady_clock::time_point::min();
    }
  }
}

void Alarms::handleActivationLimit(
    ActivationLimit& limit_info, const utils::ValueUnit& value_unit,
    const std::chrono::steady_clock::time_point now) {
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

  // If over limit, check if start event was sent or need to send continue event
  if (limit_info.is_over_limit) {
    if (limit_info.last_continue_event_sent ==
        std::chrono::steady_clock::time_point::min()) {
      sendLimitEvent(limit_info.limit_id, limit_info.fixed_peripheral_id,
                     value_unit, utils::LimitEvent::Type::kStart);
      limit_info.last_continue_event_sent = now;
    } else if (now - limit_info.last_continue_event_sent >
               continue_event_period_) {
      // Already sent start event, check if need to send continue again
      sendLimitEvent(limit_info.limit_id, limit_info.fixed_peripheral_id,
                     value_unit, utils::LimitEvent::Type::kContinue);
      limit_info.last_continue_event_sent = now;
    }
  }

  // At the end of the period, reset the counter
  if (now - limit_info.period_start > limit_info.period) {
    // Only reset over limit if under the limit for a full period
    if (limit_info.is_over_limit &&
        limit_info.highs < limit_info.highs_per_period) {
      limit_info.is_over_limit = false;
      limit_info.last_continue_event_sent =
          std::chrono::steady_clock::time_point::min();
      sendLimitEvent(limit_info.limit_id, limit_info.fixed_peripheral_id,
                     value_unit, utils::LimitEvent::Type::kEnd);
    }
    limit_info.period_start = now;
    limit_info.highs = 0;
  }
}

void Alarms::sendLimitEvent(const utils::UUID& limit_id,
                            const utils::UUID* fixed_peripheral_id,
                            const utils::ValueUnit& value_unit,
                            const utils::LimitEvent::Type type) {
  // Don't send if the controller limit ID or FP ID was not set
  if (!limit_id.isValid() || fixed_peripheral_id == nullptr) {
    Serial.printf("Limit event: %d : %s\n", int(type),
                  value_unit.data_point_type.toString().c_str());
    return;
  }

  JsonDocument limit_event;
  limit_event[WebSocket::limit_id_key_] = limit_id.toString();
  limit_event[utils::ValueUnit::value_key] = 1;
  limit_event[WebSocket::fixed_peripheral_id_key_] =
      fixed_peripheral_id->toString();
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

Alarms::BoolLimit Alarms::makeBoolLimitInfo(
    const utils::UUID* fixed_peripheral_id,
    const std::chrono::seconds delay_duration) {
  return BoolLimit{
      .is_high = false,
      .limit_id = nullptr,
      .fixed_peripheral_id = fixed_peripheral_id,
      .last_continue_event_sent = std::chrono::steady_clock::time_point::min(),
      .delay_start = std::chrono::steady_clock::time_point::min(),
      .delay_duration = delay_duration};
}

Alarms::DurationLimit Alarms::makeDurationLimitInfo(
    const utils::UUID* fixed_peripheral_id,
    const std::chrono::seconds high_duration) {
  return DurationLimit{
      .is_high = false,
      .limit_id = nullptr,
      .fixed_peripheral_id = fixed_peripheral_id,
      .last_continue_event_sent = std::chrono::steady_clock::time_point::min(),
      .high_duration = high_duration,
      .high_start = std::chrono::steady_clock::time_point::min()};
}

Alarms::ActivationLimit Alarms::makeActivationLimitInfo(
    const utils::UUID* fixed_peripheral_id, const uint32_t highs_per_period,
    const std::chrono::seconds period) {
  return ActivationLimit{
      .is_high = false,
      .limit_id = nullptr,
      .fixed_peripheral_id = fixed_peripheral_id,
      .last_continue_event_sent = std::chrono::steady_clock::time_point::min(),
      .highs_per_period = highs_per_period,
      .period = period,
      .is_over_limit = false};
}

const std::chrono::milliseconds Alarms::default_interval_{1000};

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata
#endif