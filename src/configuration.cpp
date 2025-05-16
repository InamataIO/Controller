/**
 * All user-configurations are set here
 *
 * This includes Wifi's SSID and password.
 */

#include "configuration.h"

namespace inamata {

const __FlashStringHelper* wifi_portal_ssid = FPSTR("InamataSetup");
const __FlashStringHelper* wifi_portal_password = FPSTR("12345678");

const char* GSM_APN = "wsim";

}  // namespace inamata
