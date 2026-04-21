/**
 * All user-configurations are set here
 *
 * This includes Wifi's SSID and password.
 */

#include "configuration.h"

size_t rootca_crt_bundle_len() {
  return rootca_crt_bundle_end - rootca_crt_bundle_start;
}

namespace inamata {

const __FlashStringHelper* kWifiPortalSsid = FPSTR("InamataSetup");
const __FlashStringHelper* kWifiPortalPassword = FPSTR("12345678");

const char* kGsmApn = "wsim";

}  // namespace inamata
