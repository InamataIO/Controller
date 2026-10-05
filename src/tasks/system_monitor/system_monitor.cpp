#include "system_monitor.h"

namespace inamata {
namespace tasks {
namespace system_monitor {

SystemMonitor::SystemMonitor(const ServiceGetters& services,
                             Scheduler& scheduler)
    : BaseTask(scheduler, Input(nullptr, true)),
      scheduler_(scheduler),
      services_(services) {
  setIterations(TASK_FOREVER);
}

SystemMonitor::~SystemMonitor() {}

const String& SystemMonitor::getType() const { return type(); }

const String& SystemMonitor::type() {
  static const String name{"SystemMonitor"};
  return name;
}

bool SystemMonitor::OnTaskEnable() {
  web_socket_ = services_.getWebSocket();
  if (web_socket_ == nullptr) {
    setInvalid(services_.wifi_network_nullptr_error_);
    return false;
  }
  // Reset counters to calculate CPU load. Wait one interval for valid readings
  scheduler_.cpuLoadReset();
  delay();

  return true;
}

bool SystemMonitor::TaskCallback() {
  if (!services_.getWebSocket()->isConnected()) {
    Task::delay(std::chrono::milliseconds(offline_interval_).count());
    return true;
  } else {
    Task::delay(std::chrono::milliseconds(default_interval_).count());
  }

  // Get the total available memory and largest continguous memory block.
  // This allows us to calculate the fragmentation index =
  //     (total free - largest free block) / total free * 100
  const size_t stack_hwm = uxTaskGetStackHighWaterMark(NULL) * 4;
  const size_t free_bytes = heap_caps_get_free_size(MALLOC_CAP_8BIT);
  const size_t max_malloc_bytes =
      heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
  const size_t least_free_bytes =
      heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT);
  const size_t heap_size = heap_caps_get_total_size(MALLOC_CAP_8BIT);
  uint8_t heap_fragmentation = 0;

  JsonDocument doc_out;
  if (stack_hwm) {
    doc_out["stack_hwm_bytes"] = stack_hwm;
  }
  if (free_bytes) {
    doc_out["free_memory_bytes"] = free_bytes;
  }
  if (free_bytes && max_malloc_bytes) {
    heap_fragmentation = (float(free_bytes) - float(max_malloc_bytes)) /
                         float(free_bytes) * 100.0f;
  }
  if (heap_fragmentation) {
    doc_out["heap_fragmentation_percent"] = heap_fragmentation;
  }
  if (least_free_bytes) {
    doc_out["least_free_bytes"] = least_free_bytes;
  }
  if (heap_size) {
    doc_out["heap_size"] = heap_size;
  }

  float cpuTotal = scheduler_.getCpuLoadTotal();
  float cpuCycles = scheduler_.getCpuLoadCycle();
  scheduler_.cpuLoadReset();

  doc_out["cpu_load"] = cpuCycles / cpuTotal * 100.0;

  // Get Wi-Fi and LTE connection strength
  bool set_wifi_details = true;
  const char* network_mode_key = "network_mode";

#ifdef GSM_NETWORK
  const auto gsm_network = services_.getGsmNetwork();
  if (gsm_network->isEnabled()) {
    set_wifi_details = false;
    doc_out[network_mode_key] = "mobile";
    doc_out["mobile_operator"] = gsm_network->current_mno_.c_str();
    doc_out["mobile_rssi"] = gsm_network->signal_quality_;
    doc_out["mobile_nsm"] = gsm_network->getNetworkSystemModeName();
  }
#endif

  if (set_wifi_details) {
    doc_out[network_mode_key] = "wifi";
    doc_out["wifi_ssid"] = WiFi.SSID();
    doc_out["wifi_rssi"] = WiFi.RSSI();
  }

  web_socket_->sendSystem(doc_out.as<JsonObject>());
  return true;
}

const std::chrono::seconds SystemMonitor::default_interval_{60 * 30};
const std::chrono::seconds SystemMonitor::offline_interval_{30};

}  // namespace system_monitor
}  // namespace tasks
}  // namespace inamata
