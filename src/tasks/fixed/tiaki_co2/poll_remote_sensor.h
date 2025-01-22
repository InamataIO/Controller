#include "managers/services.h"
#include "peripheral/peripherals/modbus/modbus_client_input.h"
#include "poll_abstract.h"

namespace inamata {
namespace tasks {
namespace fixed {

class PollRemoteSensor : public PollAbstract {
 public:
  using ModbusClientInput = peripheral::peripherals::modbus::ModbusClientInput;

  PollRemoteSensor(const ServiceGetters& services, Scheduler& scheduler,
                   const JsonObjectConst& behavior_config);
  virtual ~PollRemoteSensor() = default;

  const String& getType() const final;
  static const String& type();

  bool TaskCallback();

 private:
  void handleResult(std::vector<utils::ValueUnit>& values) final;

  std::shared_ptr<WebSocket> web_socket_;

  float last_co2_ppm_ = NAN;
  float last_voc_index_ = NAN;

  // Max time is ~72 minutes due to an overflow in the CPU load counter
  static const std::chrono::seconds default_interval_;
};

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata