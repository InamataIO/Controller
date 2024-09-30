/**
 * All user-configurations are set here
 *
 * This includes Wifi's SSID and password.
 */

#pragma once

#include <ArduinoJson.h>

#include <chrono>
#include <initializer_list>

#include "managers/types.h"

#ifdef ESP32
extern const uint8_t rootca_crt_bundle_start[] asm(
    "_binary_data_cert_x509_crt_bundle_bin_start");
#endif

namespace inamata {

// Connectivity
extern const __FlashStringHelper* wifi_portal_ssid;
extern const __FlashStringHelper* wifi_portal_password;
static const std::chrono::milliseconds check_connectivity_period(100);

// Connection Timeouts
static const std::chrono::seconds wifi_connect_timeout(30);
static const std::chrono::seconds web_socket_connect_timeout(30);
static const std::chrono::minutes provision_timeout(5);

}  // namespace inamata
