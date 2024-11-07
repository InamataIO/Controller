#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

namespace inamata {
namespace peripheral {
namespace fixed {

extern const __FlashStringHelper* config;
void setRegisterFixedPeripherals(JsonObject msg);

#ifdef DEVICE_TYPE_VOC_SENSOR_MK1
#define FIXED_PERIPHERALS_ACTIVE

extern const __FlashStringHelper* peripheral_voc_id;
extern const __FlashStringHelper* peripheral_air_id;
extern const __FlashStringHelper* peripheral_status_led_id;

extern const __FlashStringHelper* dpt_humidity_id;
extern const __FlashStringHelper* dpt_temperature_c_id;

#endif

}  // namespace fixed
}  // namespace peripheral
}  // namespace inamata