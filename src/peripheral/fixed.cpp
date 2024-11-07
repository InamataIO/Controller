#include "fixed.h"

namespace inamata {
namespace peripheral {
namespace fixed {

#ifdef DEVICE_TYPE_VOC_SENSOR_MK1

const __FlashStringHelper* peripheral_voc_id =
    FPSTR("fe969457-ad85-47c4-837a-20806f6f7379");
const __FlashStringHelper* peripheral_air_id =
    FPSTR("ccb2841c-4d2d-46b6-834e-ff046adbcd00");
const __FlashStringHelper* peripheral_status_led_id =
    FPSTR("1fb3a817-5310-4e14-950b-c96b4761d49c");

const __FlashStringHelper* dpt_voc_id =
    FPSTR("e22b2ea8-dd1c-4830-a6bd-b8dcfa1ba2cf");
const __FlashStringHelper* dpt_humidity_id =
    FPSTR("8843470c-79aa-4db9-944c-d02b3b6f8c35");
const __FlashStringHelper* dpt_temperature_c_id =
    FPSTR("2c87f3d4-9150-4582-a14e-4630b0779f5d");

const __FlashStringHelper* config = FPSTR(
    R"([
{"speed": 800.0, "variant": "NeoPixel_RGB", "color_encoding": "grb", "led_pin": 41.0, "led_count": 1.0, "uuid": "1fb3a817-5310-4e14-950b-c96b4761d49c", "type": "NeoPixel", "version": 0},
{"initial_state": true, "pin": 48.0, "data_point_type": "3c12e803-a1e4-4aca-885c-55329a49b4ab", "uuid": "4eac9fb8-08f6-4aab-9779-24c1bbccd31c", "type": "DigitalOut", "version": 0},
{"scl": 5.0, "sda": 4.0, "uuid": "f75bb68c-5d47-4ef1-8f5d-bdf7609f8cf0", "type": "I2CAdapter", "version": 0},
{"i2c_adapter": "f75bb68c-5d47-4ef1-8f5d-bdf7609f8cf0", "i2c_address": 89.0, "data_point_type": "e22b2ea8-dd1c-4830-a6bd-b8dcfa1ba2cf", "uuid": "fe969457-ad85-47c4-837a-20806f6f7379", "type": "SGP40", "version": 0},
{"i2c_adapter": "f75bb68c-5d47-4ef1-8f5d-bdf7609f8cf0", "i2c_address": 64.0, "humidity_data_point_type": "8843470c-79aa-4db9-944c-d02b3b6f8c35", "temperature_data_point_type": "2c87f3d4-9150-4582-a14e-4630b0779f5d", "uuid": "ccb2841c-4d2d-46b6-834e-ff046adbcd00", "type": "HDC2080", "version": 0}
])");

void setRegisterFixedPeripherals(JsonObject msg) {
  JsonArray fps = msg["fps"].to<JsonArray>();

  // Set VOC peripheral and DPT data
  JsonObject fp = fps.add<JsonObject>();
  fp["fid"] = peripheral_voc_id;
  JsonArray fdpts = fp["fdpts"].to<JsonArray>();
  JsonObject fdpt = fdpts.add<JsonObject>();
  fdpt["fid"] = dpt_voc_id;

  // Set temp./humidity sensor peripheral and DPT data
  fp = fps.add<JsonObject>();
  fp["fid"] = peripheral_air_id;
  fdpts = fp["fdpts"].to<JsonArray>();
  fdpt = fdpts.add<JsonObject>();
  fdpt["fid"] = dpt_humidity_id;
  fdpt["prefix"] = F("humidity");
  fdpt = fdpts.add<JsonObject>();
  fdpt["fid"] = dpt_temperature_c_id;
  fdpt["prefix"] = F("temperature");

  // Set status LED
  fp = fps.add<JsonObject>();
  fp["fid"] = peripheral_status_led_id;
}

#endif

#ifndef FIXED_PERIPHERALS_ACTIVE

const __FlashStringHelper* config = nullptr;
void setRegisterFixedPeripherals(JsonObject msg) {}

#endif

}  // namespace fixed
}  // namespace peripheral
}  // namespace inamata