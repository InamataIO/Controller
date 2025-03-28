#include "service_getters.h"

namespace inamata {
const __FlashStringHelper* ServiceGetters::network_nullptr_error_ =
    FPSTR("Network nullptr returned");

const __FlashStringHelper* ServiceGetters::web_socket_nullptr_error_ =
    FPSTR("WebSocket nullptr returned");

const __FlashStringHelper* ServiceGetters::storage_nullptr_error_ =
    FPSTR("Storage nullptr returned");

const __FlashStringHelper* ServiceGetters::ble_server_nullptr_error_ =
    FPSTR("BleServer nullptr returned");

const __FlashStringHelper* ServiceGetters::config_manager_nullptr_error_ =
    FPSTR("ConfigManager nullptr returned");
}  // namespace inamata
