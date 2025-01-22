
#include "managers/services.h"
#include "peripheral/peripherals/modbus/modbus_client_input.h"
#include "peripheral/peripherals/modbus/modbus_client_output.h"
#include "poll_abstract.h"
#include "tasks/base_task.h"

namespace inamata {
namespace tasks {
namespace fixed {

class AntiCondensation : public PollAbstract {
 public:
  using ModbusClientOutput =
      peripheral::peripherals::modbus::ModbusClientOutput;

  AntiCondensation(Scheduler& scheduler,
                   const JsonObjectConst& behavior_config);
  virtual ~AntiCondensation() = default;

  const String& getType() const final;
  static const String& type();

  bool TaskCallback();

 private:
  void handleResult(std::vector<utils::ValueUnit>& values) final;

  std::shared_ptr<ModbusClientOutput> modbus_output_;

  // Max time is ~72 minutes due to an overflow in the CPU load counter
  static const std::chrono::seconds default_interval_;
};

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata