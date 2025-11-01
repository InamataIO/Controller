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

const __FlashStringHelper* wifi_portal_ssid = FPSTR("InamataSetup");
const __FlashStringHelper* wifi_portal_password = FPSTR("12345678");

const char* GSM_APN = "wsim";

}  // namespace inamata
