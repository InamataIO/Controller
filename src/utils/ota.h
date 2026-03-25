#pragma once

#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>

namespace inamata {

void setup_ota(const char* hostname, const char* password);

void handle_ota();

}  // namespace inamata
