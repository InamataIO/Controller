#ifdef DEVICE_TYPE_FIRE_DATA_LOGGER

#include "log_inputs.h"

#include "managers/services.h"
#include "peripheral/fixed.h"
#include "utils/chrono_abs.h"

namespace inamata {
namespace tasks {
namespace fixed {

LogInputs::LogInputs(const ServiceGetters& services, Scheduler& scheduler)
    : BaseTask(scheduler, Input(nullptr, true)),
      logging_manager_(services.getLoggingManager()) {
  if (!isValid()) {
    return;
  }

  if (logging_manager_ == nullptr) {
    setInvalid(services.log_manager_nullptr_error_);
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
          peripheral::fixed::peripheral_maintenance_input_id));

  if (!input_bank_1_ || !input_bank_2_ || !input_bank_3_[0] ||
      !input_bank_3_[1] || !input_bank_3_[2] || !input_bank_3_[3] ||
      !input_bank_3_[4] || !input_bank_3_[5] || !input_bank_3_[6] ||
      !input_bank_3_[7] || !input_bank_3_[8]) {
    char buffer[40];
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

  setIterations(TASK_FOREVER);
  enable();
}

LogInputs::~LogInputs() {}
const String& LogInputs::getType() const { return type(); }
const String& LogInputs::type() {
  static const String name{"LogInputs"};
  return name;
}

bool LogInputs::OnTaskEnable() {
  logging_manager_->addLog("Power on");
  last_input_bank_1_state_ = input_bank_1_->getState();
  last_input_bank_2_state_ = input_bank_2_->getState();
  last_input_bank_3_state_ = getInputBank3State();

  return true;
}

bool LogInputs::TaskCallback() {
  Task::delay(std::chrono::milliseconds(default_interval_).count());
  handleDeleteLogs();

  InputBankState input_bank_1_state = input_bank_1_->getState();
  InputBankState input_bank_2_state = input_bank_2_->getState();
  InputBankState input_bank_3_state = getInputBank3State();

  processStateDiff(input_bank_1_state, last_input_bank_1_state_, 1);
  processStateDiff(input_bank_2_state, last_input_bank_2_state_, 17);
  processStateDiff(input_bank_3_state, last_input_bank_3_state_, 33);

  last_input_bank_1_state_ = input_bank_1_state;
  last_input_bank_2_state_ = input_bank_2_state;
  last_input_bank_3_state_ = input_bank_3_state;

  return true;
}

void LogInputs::processStateDiff(InputBankState currentState,
                                 InputBankState lastState, uint8_t pinOffset) {
  const InputBankState stateDiff = currentState ^ lastState;

  if (stateDiff.any()) {
    for (uint8_t i = 0; i < stateDiff.size(); i++) {
      if (stateDiff.test(i)) {
        String log_entry = 'I' + String(i + pinOffset) +
                           (currentState.test(i) ? ",ON" : ",OFF");
        logging_manager_->addLog(log_entry);
      }
    }
  }
}

LogInputs::InputBankState LogInputs::getInputBank3State() {
  InputBankState state;
  for (int i = 0; i < input_bank_3_.size(); i++) {
    state[i] = input_bank_3_[i]->readState();
  }
  return state;
}

void LogInputs::handleDeleteLogs() {
  const auto now = std::chrono::steady_clock::now();
  if (utils::chrono_abs(now - last_delete_logs_check_) >
      delete_logs_check_period_) {
    last_delete_logs_check_ = now;
    LoggingManager::deleteOldLogs();
  }
}

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata

#endif