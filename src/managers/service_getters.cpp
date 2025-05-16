#include "service_getters.h"

namespace inamata {
const __FlashStringHelper* ServiceGetters::wifi_network_nullptr_error_ =
    FPSTR("WiFiNetwork nullptr returned");
#ifdef GSM_NETWORK
const __FlashStringHelper* ServiceGetters::gsm_network_nullptr_error_ =
    FPSTR("GsmNetwork nullptr returned");
#endif
const __FlashStringHelper* ServiceGetters::web_socket_nullptr_error_ =
    FPSTR("WebSocket nullptr returned");

const __FlashStringHelper* ServiceGetters::storage_nullptr_error_ =
    FPSTR("Storage nullptr returned");

const __FlashStringHelper* ServiceGetters::ble_server_nullptr_error_ =
    FPSTR("BleServer nullptr returned");

const __FlashStringHelper* ServiceGetters::config_manager_nullptr_error_ =
    FPSTR("ConfigManager nullptr returned");

const __FlashStringHelper* ServiceGetters::log_manager_nullptr_error_ =
    FPSTR("LoggingManager nullptr returned");
}  // namespace inamata
