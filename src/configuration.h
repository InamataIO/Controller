/**
 * All user-configurations are set here
 *
 * This includes Wifi's SSID and password.
 */

#pragma once

#include <ArduinoJson.h>

#include <chrono>
#include <initializer_list>

extern const uint8_t rootca_crt_bundle_start[] asm(
    "_binary_data_cert_x509_crt_bundle_bin_start");
extern const uint8_t rootca_crt_bundle_end[] asm(
    "_binary_data_cert_x509_crt_bundle_bin_end");
size_t rootca_crt_bundle_len();

namespace inamata {

// Connectivity - WiFi
extern const __FlashStringHelper* wifi_portal_ssid;
extern const __FlashStringHelper* wifi_portal_password;
static const std::chrono::milliseconds check_connectivity_period(100);

// Conectivity - GSM
extern const char* GSM_APN;

// Connection Timeouts
static const std::chrono::seconds wifi_connect_timeout(30);
static const std::chrono::seconds web_socket_connect_timeout(30);
static const std::chrono::minutes provision_timeout(5);

#if defined(DEVICE_TYPE_VOC_SENSOR_MK1) ||    \
    defined(DEVICE_TYPE_TIAKI_CO2_MONITOR) || \
    defined(DEVICE_TYPE_FIRE_DATA_LOGGER)
#define BEHAVIOR_BASED
const bool behavior_based = true;
#endif

#ifndef BEHAVIOR_BASED
const bool behavior_based = false;
#endif

}  // namespace inamata
