#include "fixed.h"

namespace inamata {
namespace peripheral {
namespace fixed {

#ifdef FIXED_PERIPHERALS_ACTIVE

const __FlashStringHelper* dpt_button_id =
    FPSTR("78602c36-e8b8-4639-9a55-981f7bac98c9");
const __FlashStringHelper* dpt_buzzer_id =
    FPSTR("096b0b56-8f8b-420a-9db6-0701ff1ec9c5");
const __FlashStringHelper* dpt_co2_ppm_id =
    FPSTR("7f5db806-c165-48d9-b55b-176149db95d6");
const __FlashStringHelper* dpt_current_a_id =
    FPSTR("ec365eba-2b8e-4351-97ef-e68ec2d26040");
const __FlashStringHelper* dpt_heater_id =
    FPSTR("43b8759b-2f34-46b3-b47d-a59a68296517");
const __FlashStringHelper* dpt_humidity_rh_id =
    FPSTR("8843470c-79aa-4db9-944c-d02b3b6f8c35");
const __FlashStringHelper* dpt_led_id =
    FPSTR("33cc94d0-5f0b-4846-921a-0d3f0280bc85");
const __FlashStringHelper* dpt_relay_id =
    FPSTR("68d4ce65-7c21-4ff3-be5e-160e7943573a");
const __FlashStringHelper* dpt_temperature_c_id =
    FPSTR("2c87f3d4-9150-4582-a14e-4630b0779f5d");
const __FlashStringHelper* dpt_voc_index_id =
    FPSTR("e22b2ea8-dd1c-4830-a6bd-b8dcfa1ba2cf");

#endif

#ifdef DEVICE_TYPE_VOC_SENSOR_MK1

const __FlashStringHelper* peripheral_voc_id =
    FPSTR("fe969457-ad85-47c4-837a-20806f6f7379");
const __FlashStringHelper* peripheral_air_id =
    FPSTR("ccb2841c-4d2d-46b6-834e-ff046adbcd00");
const __FlashStringHelper* peripheral_status_led_id =
    FPSTR("1fb3a817-5310-4e14-950b-c96b4761d49c");

const __FlashStringHelper* config = FPSTR(
    R"([
{"speed": 800.0, "variant": "NeoPixel_RGB", "color_encoding": "grb", "led_pin": 41.0, "led_count": 1.0, "uuid": "1fb3a817-5310-4e14-950b-c96b4761d49c", "type": "NeoPixel", "version": 0},
{"initial_state": true, "pin": 48.0, "data_point_type": "3c12e803-a1e4-4aca-885c-55329a49b4ab", "uuid": "4eac9fb8-08f6-4aab-9779-24c1bbccd31c", "type": "DigitalOut", "version": 0},
{"scl": 5.0, "sda": 4.0, "uuid": "f75bb68c-5d47-4ef1-8f5d-bdf7609f8cf0", "type": "I2CAdapter", "version": 0},
{"i2c_adapter": "f75bb68c-5d47-4ef1-8f5d-bdf7609f8cf0", "i2c_address": 89.0, "data_point_type": "e22b2ea8-dd1c-4830-a6bd-b8dcfa1ba2cf", "uuid": "fe969457-ad85-47c4-837a-20806f6f7379", "type": "SGP40", "version": 0},
{"i2c_adapter": "f75bb68c-5d47-4ef1-8f5d-bdf7609f8cf0", "i2c_address": 64.0, "humidity_data_point_type": "8843470c-79aa-4db9-944c-d02b3b6f8c35", "temperature_data_point_type": "2c87f3d4-9150-4582-a14e-4630b0779f5d", "uuid": "ccb2841c-4d2d-46b6-834e-ff046adbcd00", "type": "HDC2080", "version": 0}
])");

std::array<const __FlashStringHelper*, 2> configs{config, nullptr};

void setRegisterFixedPeripherals(JsonObject msg) {
  JsonArray fps = msg["fps"].to<JsonArray>();

  // Set VOC peripheral and DPT data
  JsonObject fp = fps.add<JsonObject>();
  fp["fid"] = peripheral_voc_id;
  JsonArray fdpts = fp["fdpts"].to<JsonArray>();
  JsonObject fdpt = fdpts.add<JsonObject>();
  fdpt["fid"] = dpt_voc_index_id;

  // Set temp./humidity sensor peripheral and DPT data
  fp = fps.add<JsonObject>();
  fp["fid"] = peripheral_air_id;
  fdpts = fp["fdpts"].to<JsonArray>();
  fdpt = fdpts.add<JsonObject>();
  fdpt["fid"] = dpt_humidity_rh_id;
  fdpt["prefix"] = F("humidity");
  fdpt = fdpts.add<JsonObject>();
  fdpt["fid"] = dpt_temperature_c_id;
  fdpt["prefix"] = F("temperature");

  // Set status LED
  fp = fps.add<JsonObject>();
  fp["fid"] = peripheral_status_led_id;
}

#elif defined(DEVICE_TYPE_TIAKI_CO2_MONITOR)

const __FlashStringHelper* peripheral_led_fault_id =
    FPSTR("d6b77a3f-9ed0-40e4-af72-b4b2568f7882");
const __FlashStringHelper* peripheral_led_alarm_1_id =
    FPSTR("4da21546-078d-4e0d-a036-e20fb078627f");
const __FlashStringHelper* peripheral_led_alarm_2_id =
    FPSTR("49dd5e23-f604-4005-9358-136c0e38bb9c");
const __FlashStringHelper* peripheral_led_alarm_3_id =
    FPSTR("f0d80afb-c621-4744-87dc-69b6d43db857");
const __FlashStringHelper* peripheral_led_alarm_4_id =
    FPSTR("4cf0df60-d90d-4852-aad7-e7f92af378ba");
const __FlashStringHelper* peripheral_led_network_id =
    FPSTR("f163f9e8-2061-4064-ae11-77848ad80fb5");

const __FlashStringHelper* peripheral_touch_1_id =
    FPSTR("3b3a7020-1d56-4936-a8d4-ec175c56d824");
const __FlashStringHelper* peripheral_touch_2_id =
    FPSTR("3786e671-d0f3-4cd1-8bb2-9f848b2c328d");

const __FlashStringHelper* peripheral_buzzer_id =
    FPSTR("d9d6f349-7e99-4d98-b101-1716b677e60b");

const __FlashStringHelper* peripheral_relay_1_id =
    FPSTR("3a5aad16-e08e-4133-8fd6-0fa97ba7ba66");
const __FlashStringHelper* peripheral_relay_2_id =
    FPSTR("80c226b8-e139-4ae9-9236-7387279203ac");
const __FlashStringHelper* peripheral_relay_3_id =
    FPSTR("ff0d15a8-0fd0-4f5a-b9e6-c4990d2b7a4b");
const __FlashStringHelper* peripheral_relay_4_id =
    FPSTR("475904d6-bd33-4392-84dd-5937a97cd7db");

const __FlashStringHelper* peripheral_i2c_adapter_id =
    FPSTR("e5890186-337f-4128-bd2f-aa20be0c1afc");
const __FlashStringHelper* peripheral_analog_out_id =
    FPSTR("07f0907d-40f0-4027-b71f-c419eb6b6ac0");

const __FlashStringHelper* peripheral_modbus_client_id =
    FPSTR("d4a92657-d4bc-490a-95e6-2ff9113c4852");
const __FlashStringHelper* peripheral_modbus_sensor_in_id =
    FPSTR("42bf607e-8079-4683-981c-1748acd4f703");
const __FlashStringHelper* peripheral_modbus_sensor_out_id =
    FPSTR("9499bab8-9809-431d-927f-fe42b94aa5dd");

const __FlashStringHelper* config_1 = FPSTR(
    R"([
{"pin":42,"dpt":"33cc94d0-5f0b-4846-921a-0d3f0280bc85","uuid":"d6b77a3f-9ed0-40e4-af72-b4b2568f7882","type":"DigitalOut"},
{"pin":8,"dpt":"33cc94d0-5f0b-4846-921a-0d3f0280bc85","uuid":"4da21546-078d-4e0d-a036-e20fb078627f","type":"DigitalOut"},
{"pin":16,"dpt":"33cc94d0-5f0b-4846-921a-0d3f0280bc85","uuid":"49dd5e23-f604-4005-9358-136c0e38bb9c","type":"DigitalOut"},
{"pin":15,"dpt":"33cc94d0-5f0b-4846-921a-0d3f0280bc85","uuid":"f0d80afb-c621-4744-87dc-69b6d43db857","type":"DigitalOut"},
{"pin":7,"dpt":"33cc94d0-5f0b-4846-921a-0d3f0280bc85","uuid":"4cf0df60-d90d-4852-aad7-e7f92af378ba","type":"DigitalOut"},
{"pin":6,"dpt":"33cc94d0-5f0b-4846-921a-0d3f0280bc85","uuid":"f163f9e8-2061-4064-ae11-77848ad80fb5","type":"DigitalOut"},
{"pin":39,"input_type":"floating","dpt":"78602c36-e8b8-4639-9a55-981f7bac98c9","uuid":"3b3a7020-1d56-4936-a8d4-ec175c56d824","type":"DigitalIn"},
{"pin":40,"input_type":"floating","dpt":"78602c36-e8b8-4639-9a55-981f7bac98c9","uuid":"3786e671-d0f3-4cd1-8bb2-9f848b2c328d","type":"DigitalIn"},
{"pin":10,"dpt":"096b0b56-8f8b-420a-9db6-0701ff1ec9c5","uuid":"d9d6f349-7e99-4d98-b101-1716b677e60b","type":"DigitalOut"},
{"pin":11,"dpt":"68d4ce65-7c21-4ff3-be5e-160e7943573a","uuid":"3a5aad16-e08e-4133-8fd6-0fa97ba7ba66","type":"DigitalOut"},
{"pin":12,"dpt":"68d4ce65-7c21-4ff3-be5e-160e7943573a","uuid":"80c226b8-e139-4ae9-9236-7387279203ac","type":"DigitalOut"},
{"pin":13,"dpt":"68d4ce65-7c21-4ff3-be5e-160e7943573a","uuid":"ff0d15a8-0fd0-4f5a-b9e6-c4990d2b7a4b","type":"DigitalOut"},
{"pin":14,"dpt":"68d4ce65-7c21-4ff3-be5e-160e7943573a","uuid":"475904d6-bd33-4392-84dd-5937a97cd7db","type":"DigitalOut"},
{"scl":5,"sda":4,"uuid":"e5890186-337f-4128-bd2f-aa20be0c1afc","type":"I2CAdapter"},
{"variant":"GP8302","i2c_adapter":"e5890186-337f-4128-bd2f-aa20be0c1afc","dpt":"ec365eba-2b8e-4351-97ef-e68ec2d26040","uuid":"07f0907d-40f0-4027-b71f-c419eb6b6ac0","type":"GP8XXX"}
])");

const __FlashStringHelper* config_2 = FPSTR(
    R"([
{"rx":18,"tx":17,"dere":21,"baud_rate":9600,"uuid":"d4a92657-d4bc-490a-95e6-2ff9113c4852","type":"ModbusClientAdapter"},
{"server":5,"address":1,"size":4,"inputs":[{"dpt":"2c87f3d4-9150-4582-a14e-4630b0779f5d","offset":0,"m":0.1},{"dpt":"8843470c-79aa-4db9-944c-d02b3b6f8c35","offset":1},{"dpt":"e22b2ea8-dd1c-4830-a6bd-b8dcfa1ba2cf","offset":2},{"dpt":"7f5db806-c165-48d9-b55b-176149db95d6","offset":3}],"uuid":"42bf607e-8079-4683-981c-1748acd4f703","adapter":"d4a92657-d4bc-490a-95e6-2ff9113c4852","type":"ModbusClientInput"},
{"server":5,"outputs":[{"dpt":"43b8759b-2f34-46b3-b47d-a59a68296517","address":2}],"uuid":"9499bab8-9809-431d-927f-fe42b94aa5dd","adapter":"d4a92657-d4bc-490a-95e6-2ff9113c4852","type":"ModbusClientOutput"}
])");

std::array<const __FlashStringHelper*, 2> configs{config_1, config_2};

void addDptToJson(const __FlashStringHelper* dpt,
                  const __FlashStringHelper* prefix, JsonArray fdpts) {
  JsonObject fdpt = fdpts.add<JsonObject>();
  fdpt["fid"] = dpt;
  fdpt["prefix"] = prefix;
}

void setRegisterFixedPeripherals(JsonObject msg) {
  JsonArray fps = msg["fps"].to<JsonArray>();

  // Set CO2, VOC, temperature and humidity DPTs for remote sensor
  JsonObject fp = fps.add<JsonObject>();
  fp["fid"] = peripheral_modbus_sensor_in_id;
  JsonArray fdpts = fp["fdpts"].to<JsonArray>();
  addDptToJson(dpt_co2_ppm_id, F("co2"), fdpts);
  addDptToJson(dpt_voc_index_id, F("voc"), fdpts);
  addDptToJson(dpt_humidity_rh_id, F("humidity"), fdpts);
  addDptToJson(dpt_temperature_c_id, F("temperature"), fdpts);
}

#else

std::array<const __FlashStringHelper*, 2> configs{nullptr, nullptr};
void setRegisterFixedPeripherals(JsonObject msg) {}

#endif

}  // namespace fixed
}  // namespace peripheral
}  // namespace inamata