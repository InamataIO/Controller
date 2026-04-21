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
extern const __FlashStringHelper* kWifiPortalSsid;
extern const __FlashStringHelper* kWifiPortalPassword;
static const std::chrono::milliseconds kCheckConnectivityPeriod(100);

// Conectivity - GSM
extern const char* kGsmApn;

// Connection Timeouts
static const std::chrono::seconds kWifiConnectTimeout(30);
static const std::chrono::seconds kGsmConnectTimeout(180);
static const std::chrono::seconds kWebSocketConnectTimeout(30);
static const std::chrono::minutes kProvisionTimeout(10);

#if defined(DEVICE_TYPE_VOC_SENSOR_MK1) ||    \
    defined(DEVICE_TYPE_TIAKI_CO2_MONITOR) || \
    defined(DEVICE_TYPE_FIRE_DATA_LOGGER)
#define BEHAVIOR_BASED
const bool kBehaviorBased = true;
#endif

#ifndef BEHAVIOR_BASED
const bool kBehaviorBased = false;
#endif

}  // namespace inamata
