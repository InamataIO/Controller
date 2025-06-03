#pragma once

#include <TaskSchedulerDeclarations.h>

#include <chrono>

#include "managers/service_getters.h"
#include "peripheral/peripherals/digital_in/digital_in.h"
#include "peripheral/peripherals/pca9539/pca9539.h"
#include "tasks/base_task.h"

namespace inamata {
namespace tasks {
namespace fixed {

class LogInputs : public BaseTask {
 public:
  static constexpr size_t kInputBankSize = 16;
  using InputBankState = std::bitset<kInputBankSize>;

  using DigitalIn = peripheral::peripherals::digital_in::DigitalIn;
  using PCA9539 = peripheral::peripherals::pca9539::PCA9539;

  LogInputs(const ServiceGetters& services, Scheduler& scheduler);
  virtual ~LogInputs();

  const String& getType() const final;
  static const String& type();

 private:
  InputBankState last_input_bank_1_state_;
  InputBankState last_input_bank_2_state_;
  InputBankState last_input_bank_3_state_;

  bool OnTaskEnable() final;
  bool TaskCallback() final;

  void processStateDiff(InputBankState currentState, InputBankState lastState,
                        uint8_t pinOffset);
  InputBankState getInputBank3State();

  std::shared_ptr<PCA9539> input_bank_1_;
  std::shared_ptr<PCA9539> input_bank_2_;
  std::array<std::shared_ptr<DigitalIn>, 9> input_bank_3_;

  std::shared_ptr<LoggingManager> logging_manager_;

  /// Max time is ~72 minutes due to an overflow in the CPU load counter
  static constexpr std::chrono::milliseconds default_interval_{1000};
};

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata
