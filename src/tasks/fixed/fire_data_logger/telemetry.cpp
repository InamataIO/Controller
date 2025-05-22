#ifdef DEVICE_TYPE_FIRE_DATA_LOGGER

#include "telemetry.h"

#include "utils/chrono_abs.h"

namespace inamata {
namespace tasks {
namespace fixed {

Telemetry::Telemetry(const ServiceGetters& services, Scheduler& scheduler)
    : BaseTask(scheduler, Input(nullptr, true)),
      web_socket_(services.getWebSocket()) {
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
  input_bank_3_[8] =
      std::dynamic_pointer_cast<DigitalIn>(peripheral_controller.getPeripheral(
          peripheral::fixed::peripheral_i41_id));

  if (!input_bank_1_ || !input_bank_2_ || !input_bank_3_[0] ||
      !input_bank_3_[1] || !input_bank_3_[2] || !input_bank_3_[3] ||
      !input_bank_3_[4] || !input_bank_3_[5] || !input_bank_3_[6] ||
      !input_bank_3_[7] || !input_bank_3_[8]) {
    char buffer[46];
    const int len = snprintf(
        buffer, sizeof(buffer),
        "Missing peri: %d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d", bool(input_bank_1_),
        bool(input_bank_2_), bool(input_bank_3_[0]), bool(input_bank_3_[1]),
        bool(input_bank_3_[2]), bool(input_bank_3_[3]), bool(input_bank_3_[4]),
        bool(input_bank_3_[5]), bool(input_bank_3_[6]), bool(input_bank_3_[7]),
        bool(input_bank_3_[8]));
    setInvalid(buffer);
    return;
  }

  if (web_socket_ == nullptr) {
    setInvalid(services.web_socket_nullptr_error_);
    return;
  }

  setIterations(TASK_FOREVER);
  enable();
}

const String& Telemetry::getType() const { return type(); }

const String& Telemetry::type() {
  static const String name{"Telemetry"};
  return name;
}

bool Telemetry::TaskCallback() {
  Task::delay(std::chrono::milliseconds(default_interval_).count());

  // Move current to previous states to set current states. Extract the value of
  // all inputs. Compare at end to previous state.
  std::bitset<kInputCount> current_states;
  auto result = input_bank_1_->getValues();
  for (const auto& value : result.values) {
    saveResult(value, current_states);
  }
  result = input_bank_2_->getValues();
  for (const auto& value : result.values) {
    saveResult(value, current_states);
  }
  for (const auto& digital_in : input_bank_3_) {
    result = digital_in->getValues();
    if (result.values.size() == 1) {
      saveResult(result.values[0], current_states);
    } else {
      TRACEF("Missing value: %s", result.error.toString().c_str());
    }
  }

  // Send all changed input immediately
  std::bitset<kInputCount> diff = previous_states_ ^ current_states;
  sendTelemetry(current_states, diff);

  // Send all inputs once an hour if connected
  const auto now = std::chrono::steady_clock::now();
  if (utils::chrono_abs(now - last_full_send_) > full_send_period_) {
    last_full_send_ = now;
    if (web_socket_->isConnected()) {
      sendTelemetry(current_states, std::bitset<kInputCount>{}.set());
    }
  }

  previous_states_ = current_states;
  return true;
}

void Telemetry::saveResult(const utils::ValueUnit& value_unit,
                           std::bitset<kInputCount>& current_states) {
  for (size_t i = 0; i < dpts_.size(); i++) {
    if (value_unit.data_point_type == *dpts_[i]) {
      current_states[i] = value_unit.value > 0.5;
      break;
    }
  }
}

void Telemetry::sendTelemetry(const std::bitset<kInputCount>& current_states,
                              const std::bitset<kInputCount>& diff) {
  JsonDocument doc_out;
  {
    // Send changes for IO Bank 1
    JsonObject result_object = doc_out.to<JsonObject>();
    std::vector<utils::ValueUnit> values;
    for (size_t i = 0; i < 16; i++) {
      if (diff.test(i)) {
        values.emplace_back(current_states[i], *dpts_[i]);
      }
    }
    if (values.size()) {
      WebSocket::packageTelemetry(values, peripheral::fixed::peripheral_io_1_id,
                                  true, result_object);
      web_socket_->sendTelemetry(result_object);
    }
  }
  {
    // Send changes for IO Bank 2
    JsonObject result_object = doc_out.to<JsonObject>();
    std::vector<utils::ValueUnit> values;
    for (size_t i = 16; i < 32; i++) {
      if (diff.test(i)) {
        values.emplace_back(current_states[i], *dpts_[i]);
      }
    }
    if (values.size()) {
      WebSocket::packageTelemetry(values, peripheral::fixed::peripheral_io_2_id,
                                  true, result_object);
      web_socket_->sendTelemetry(result_object);
    }
  }

  // Send changes for IO Bank 3
  std::vector<inamata::utils::ValueUnit> value_unit(1);
  for (size_t i = 32; i < dpts_.size(); i++) {
    if (!diff.test(i)) {
      continue;
    }
    JsonObject result_object = doc_out.to<JsonObject>();
    value_unit[0].data_point_type = *dpts_[i];
    value_unit[0].value = current_states[i];
    WebSocket::packageTelemetry(value_unit, input_bank_3_[i - 32]->id, true,
                                result_object);
    web_socket_->sendTelemetry(result_object);
  }
}

const std::array<const utils::UUID*, kInputCount> Telemetry::dpts_ = {
    &peripheral::fixed::dpt_diesel_1_fire_alarm_id,
    &peripheral::fixed::dpt_diesel_2_fire_alarm_id,
    &peripheral::fixed::dpt_diesel_3_fire_alarm_id,
    &peripheral::fixed::dpt_diesel_4_fire_alarm_id,
    &peripheral::fixed::dpt_diesel_1_pump_run_id,
    &peripheral::fixed::dpt_diesel_2_pump_run_id,
    &peripheral::fixed::dpt_diesel_3_pump_run_id,
    &peripheral::fixed::dpt_diesel_4_pump_run_id,
    &peripheral::fixed::dpt_diesel_1_pump_fail_id,
    &peripheral::fixed::dpt_diesel_2_pump_fail_id,
    &peripheral::fixed::dpt_diesel_3_pump_fail_id,
    &peripheral::fixed::dpt_diesel_4_pump_fail_id,
    &peripheral::fixed::dpt_diesel_1_battery_charger_fail_id,
    &peripheral::fixed::dpt_diesel_2_battery_charger_fail_id,
    &peripheral::fixed::dpt_diesel_3_battery_charger_fail_id,
    &peripheral::fixed::dpt_diesel_4_battery_charger_fail_id,
    &peripheral::fixed::dpt_diesel_1_low_oil_level_fail_id,
    &peripheral::fixed::dpt_diesel_2_low_oil_level_fail_id,
    &peripheral::fixed::dpt_diesel_3_low_oil_level_fail_id,
    &peripheral::fixed::dpt_diesel_4_low_oil_level_fail_id,
    &peripheral::fixed::dpt_diesel_control_circuit_fail_id,
    &peripheral::fixed::dpt_diesel_mains_fail_id,
    &peripheral::fixed::dpt_diesel_pump_fail_id,
    &peripheral::fixed::dpt_diesel_engine_overheat_fail_id,
    &peripheral::fixed::dpt_diesel_fuel_tank_low_id,
    &peripheral::fixed::dpt_electric_1_fire_alarm_id,
    &peripheral::fixed::dpt_electric_2_fire_alarm_id,
    &peripheral::fixed::dpt_electric_1_pump_run_id,
    &peripheral::fixed::dpt_electric_2_pump_run_id,
    &peripheral::fixed::dpt_electric_1_pump_fail_id,
    &peripheral::fixed::dpt_electric_2_pump_fail_id,
    &peripheral::fixed::dpt_electric_mains_fail_id,
    &peripheral::fixed::dpt_electric_control_circuit_fail_id,
    &peripheral::fixed::dpt_jockey_1_pump_run_id,
    &peripheral::fixed::dpt_jockey_2_pump_run_id,
    &peripheral::fixed::dpt_jockey_1_pump_fail_id,
    &peripheral::fixed::dpt_jockey_2_pump_fail_id,
    &peripheral::fixed::dpt_pumphouse_protection_alarm_id,
    &peripheral::fixed::dpt_annunciator_fault_id,
    &peripheral::fixed::dpt_pumphouse_flooding_alarm_id,
    &peripheral::fixed::dpt_i41_id,
};

const std::chrono::milliseconds Telemetry::default_interval_{1000};

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata

#endif