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

#elif defined(DEVICE_TYPE_FIRE_DATA_LOGGER)

const utils::UUID peripheral_i2c_adapter_id =
    "2f99965f-ce6d-44dc-81cb-75f9e6afaf3c";
// PCA9539 1: 16 pins
const utils::UUID peripheral_io_1_id = "bccf2795-7533-41ef-b3d2-f1d587c86a32";
// PCA9539 2: 16 pins
const utils::UUID peripheral_io_2_id = "1e75c20c-a565-4cd0-b8ac-9629ca837515";
// PCA9536D 1: 3 pins (of 4)
const utils::UUID peripheral_io_3_id = "4f1da40b-e239-4fc8-87fd-29b7ac6db5b7";

const utils::UUID peripheral_electric_control_circuit_fail_id =
    "8ac7fcc4-e7ae-4bf4-a303-979b5fae3fa1";

const utils::UUID peripheral_jockey_1_pump_run_id =
    "0c610345-fe55-4caa-ae63-231e0088060a";
const utils::UUID peripheral_jockey_2_pump_run_id =
    "55d4b7b1-f8d9-4370-8c89-26db5209a113";
const utils::UUID peripheral_jockey_1_pump_fail_id =
    "d213edba-4756-46ee-85c1-bbfd2121691b";
const utils::UUID peripheral_jockey_2_pump_fail_id =
    "69df61ed-f528-4974-b084-d5b70be71d76";

const utils::UUID peripheral_pumphouse_protection_alarm_id =
    "7817677d-9e22-4721-b5ee-9b138103cbd7";
const utils::UUID peripheral_annunciator_fault_id =
    "76581861-7c3b-4480-ab60-44682c8d606b";
const utils::UUID peripheral_pumphouse_flooding_alarm_id =
    "90d8fd97-5cc7-4241-9c60-f6c0f6f79287";
const utils::UUID peripheral_i41_id = "6e3f58b2-89ba-4e4c-9a7a-8eaec27c0ed5";

const utils::UUID peripheral_relay_1_id =
    "3faabb21-e3f0-49bb-a84c-3e47a6e935fb";
const utils::UUID peripheral_relay_2_id =
    "4a6a9a0c-3af3-4c1c-84df-0f35c4ad58c4";

const utils::UUID peripheral_status_led_id =
    "2048bc79-2caf-4265-a713-4a5b5fa26394";

// Ports I1-4 (PCA9539: 1 pin 0-3)
const utils::UUID dpt_diesel_1_fire_alarm_id =
    "fa9939f9-7d7b-4303-8ba5-7718afa369f0";
const utils::UUID dpt_diesel_2_fire_alarm_id =
    "5b6eebcc-7017-4aae-b37c-8cca0181ae94";
const utils::UUID dpt_diesel_3_fire_alarm_id =
    "c3338cca-2fa5-4b95-b002-13b33176ccbc";
const utils::UUID dpt_diesel_4_fire_alarm_id =
    "a1d5f7e3-194a-4c3a-963d-fcfa14464dbf";

// Ports I5-8 (PCA9539: 1 pin 4-7)
const utils::UUID dpt_diesel_1_pump_run_id =
    "1efb6f6a-50a1-4267-84a9-f800fcae412b";
const utils::UUID dpt_diesel_2_pump_run_id =
    "f34bcce7-6d5d-4b6a-ac4d-7fd028dcff7c";
const utils::UUID dpt_diesel_3_pump_run_id =
    "12be15ec-d71c-4420-a3ec-d9be49234152";
const utils::UUID dpt_diesel_4_pump_run_id =
    "5da7c712-d286-4ae3-ae56-639a8c42a75e";

// Ports I9-12 (PCA9539: 1 pin 8-11)
const utils::UUID dpt_diesel_1_pump_fail_id =
    "a982c7e2-59dd-4cc4-8674-20bd3e452d65";
const utils::UUID dpt_diesel_2_pump_fail_id =
    "d4e05f90-3774-4ea8-9dc7-df3440c49f3e";
const utils::UUID dpt_diesel_3_pump_fail_id =
    "454ba26c-09c1-4a6c-b2bb-69038c6e912c";
const utils::UUID dpt_diesel_4_pump_fail_id =
    "e82c196c-4f4f-4471-9d9b-f20725fc51d4";

// Ports I13-16 (PCA9539: 1 pin 12-15)
const utils::UUID dpt_diesel_1_battery_charger_fail_id =
    "7173899b-8a85-4cf3-8721-5dbc23658c3a";
const utils::UUID dpt_diesel_2_battery_charger_fail_id =
    "3ad7b7aa-dfb1-43b1-96ec-8071b067824b";
const utils::UUID dpt_diesel_3_battery_charger_fail_id =
    "9de56ad6-2e71-4913-9c1b-241ebc352846";
const utils::UUID dpt_diesel_4_battery_charger_fail_id =
    "4aa525d1-e217-465f-ab18-73af003d90c1";

// Ports I17-120 (PCA9539: 2 pin 0-3)
const utils::UUID dpt_diesel_1_low_oil_level_fail_id =
    "db72aeb2-1907-452b-b7bb-b3b70e511504";
const utils::UUID dpt_diesel_2_low_oil_level_fail_id =
    "b311d288-2f75-44d1-9130-a44eb97b038c";
const utils::UUID dpt_diesel_3_low_oil_level_fail_id =
    "ece604b9-0ef5-4ec1-bc3f-7072623db190";
const utils::UUID dpt_diesel_4_low_oil_level_fail_id =
    "459e1ae2-cfb5-4b7d-9476-cac367655988";

// Ports I21-125 (PCA9539: 2 pin 4-8)
const utils::UUID dpt_diesel_control_circuit_fail_id =
    "0a2ba814-e6d6-4fab-9273-55cdfe646126";
const utils::UUID dpt_diesel_mains_fail_id =
    "72846fa0-1a08-4569-955c-71876b348edf";
const utils::UUID dpt_diesel_pump_fail_id =
    "1b677e95-3b8d-4a0d-ab11-c6c327de16d2";
const utils::UUID dpt_diesel_engine_overheat_fail_id =
    "fff6ea3d-8c46-488d-a0ad-f4b21cb85de7";
const utils::UUID dpt_diesel_fuel_tank_low_id =
    "8f66c61e-af0c-43dc-a0a3-dd8f661ec3d8";

// Ports I26-132 (PCA9539: 2 pin 9-15)
const utils::UUID dpt_electric_1_fire_alarm_id =
    "2a0b97ae-d896-4cd7-bc92-050179f8ce7a";
const utils::UUID dpt_electric_2_fire_alarm_id =
    "0e5d33d9-98d7-476b-9a9c-e586979e1173";
const utils::UUID dpt_electric_1_pump_run_id =
    "080ed05b-3e22-4437-bab0-4df783a84270";
const utils::UUID dpt_electric_2_pump_run_id =
    "2237695c-2166-42d3-a518-7b701c1baeb0";
const utils::UUID dpt_electric_1_pump_fail_id =
    "ae393474-019e-419b-a183-981f605f379d";
const utils::UUID dpt_electric_2_pump_fail_id =
    "a2d852a2-ddb4-4f6b-b753-008a2fd324b2";
const utils::UUID dpt_electric_mains_fail_id =
    "4aef53e1-d90d-42a0-bd2d-dab3d76e61ae";
// Ports I33 (GPIO 42)
const utils::UUID dpt_electric_control_circuit_fail_id =
    "3586608f-5d26-478a-abc2-223fd70664e8";

// Ports I34-I37 (GPIO 41,40,39,38)
const utils::UUID dpt_jockey_1_pump_run_id =
    "772b4ba0-e32e-48fa-ac28-2c464b4f25d2";
const utils::UUID dpt_jockey_2_pump_run_id =
    "7a6ff5d9-b979-4318-b3af-5e76876d6ca7";
const utils::UUID dpt_jockey_1_pump_fail_id =
    "070c5e6e-2487-41c8-93b5-be9adad42af5";
const utils::UUID dpt_jockey_2_pump_fail_id =
    "a8274676-bf67-409b-a364-bddfeb18884a";

// Ports I38-I41 (GPIO 48,47,21,6)
const utils::UUID dpt_pumphouse_protection_alarm_id =
    "d21f228f-ef71-4d10-8ef6-d3b415e207e6";
const utils::UUID dpt_annunciator_fault_id =
    "4bc1b31d-3610-45bf-baf0-503afd45f6b5";
const utils::UUID dpt_pumphouse_flooding_alarm_id =
    "25a58394-1d7b-4645-8e08-a47952a49c1e";
const utils::UUID dpt_i41_id = "cd9f1cd2-0252-4c6e-b9eb-7868de72face";

// Maintenance button
const utils::UUID dpt_mem_wr_led_id = "7699a3b3-ebf5-444a-8cd8-49776dba5f5d";
const utils::UUID dpt_gsm_wifi_toggle_id =
    "07fafa6a-b31a-431e-8510-09d5212ecefc";
const utils::UUID dpt_maintenance_mode_id =
    "0dab65a4-92fa-420d-990c-eaf82da508ea";

const __FlashStringHelper* config_1 = FPSTR(R"([
{"type":"I2CAdapter","uuid":"2f99965f-ce6d-44dc-81cb-75f9e6afaf3c","scl":9,"sda":8},
{"type":"PCA9539","uuid":"bccf2795-7533-41ef-b3d2-f1d587c86a32","i2c_adapter":"2f99965f-ce6d-44dc-81cb-75f9e6afaf3c","i2c_address":116,"reset":45,"active_low_in":true,"inputs":[{"pin":0,"dpt":"fa9939f9-7d7b-4303-8ba5-7718afa369f0"},{"pin":1,"dpt":"5b6eebcc-7017-4aae-b37c-8cca0181ae94"},{"pin":2,"dpt":"c3338cca-2fa5-4b95-b002-13b33176ccbc"},{"pin":3,"dpt":"a1d5f7e3-194a-4c3a-963d-fcfa14464dbf"},{"pin":4,"dpt":"1efb6f6a-50a1-4267-84a9-f800fcae412b"},{"pin":5,"dpt":"f34bcce7-6d5d-4b6a-ac4d-7fd028dcff7c"},{"pin":6,"dpt":"12be15ec-d71c-4420-a3ec-d9be49234152"},{"pin":7,"dpt":"5da7c712-d286-4ae3-ae56-639a8c42a75e"},{"pin":8,"dpt":"a982c7e2-59dd-4cc4-8674-20bd3e452d65"},{"pin":9,"dpt":"d4e05f90-3774-4ea8-9dc7-df3440c49f3e"},{"pin":10,"dpt":"454ba26c-09c1-4a6c-b2bb-69038c6e912c"},{"pin":11,"dpt":"e82c196c-4f4f-4471-9d9b-f20725fc51d4"},{"pin":12,"dpt":"7173899b-8a85-4cf3-8721-5dbc23658c3a"},{"pin":13,"dpt":"3ad7b7aa-dfb1-43b1-96ec-8071b067824b"},{"pin":14,"dpt":"9de56ad6-2e71-4913-9c1b-241ebc352846"},{"pin":15,"dpt":"4aa525d1-e217-465f-ab18-73af003d90c1"}]},
{"type":"PCA9539","uuid":"1e75c20c-a565-4cd0-b8ac-9629ca837515","i2c_adapter":"2f99965f-ce6d-44dc-81cb-75f9e6afaf3c","i2c_address":117,"reset":45,"active_low_in":true,"inputs":[{"pin":0,"dpt":"db72aeb2-1907-452b-b7bb-b3b70e511504"},{"pin":1,"dpt":"b311d288-2f75-44d1-9130-a44eb97b038c"},{"pin":2,"dpt":"ece604b9-0ef5-4ec1-bc3f-7072623db190"},{"pin":3,"dpt":"459e1ae2-cfb5-4b7d-9476-cac367655988"},{"pin":4,"dpt":"0a2ba814-e6d6-4fab-9273-55cdfe646126"},{"pin":5,"dpt":"72846fa0-1a08-4569-955c-71876b348edf"},{"pin":6,"dpt":"1b677e95-3b8d-4a0d-ab11-c6c327de16d2"},{"pin":7,"dpt":"fff6ea3d-8c46-488d-a0ad-f4b21cb85de7"},{"pin":8,"dpt":"8f66c61e-af0c-43dc-a0a3-dd8f661ec3d8"},{"pin":9,"dpt":"2a0b97ae-d896-4cd7-bc92-050179f8ce7a"},{"pin":10,"dpt":"0e5d33d9-98d7-476b-9a9c-e586979e1173"},{"pin":11,"dpt":"080ed05b-3e22-4437-bab0-4df783a84270"},{"pin":12,"dpt":"2237695c-2166-42d3-a518-7b701c1baeb0"},{"pin":13,"dpt":"ae393474-019e-419b-a183-981f605f379d"},{"pin":14,"dpt":"a2d852a2-ddb4-4f6b-b753-008a2fd324b2"},{"pin":15,"dpt":"4aef53e1-d90d-42a0-bd2d-dab3d76e61ae"}]},
{"type":"PCA9536D","uuid":"4f1da40b-e239-4fc8-87fd-29b7ac6db5b7","i2c_adapter":"2f99965f-ce6d-44dc-81cb-75f9e6afaf3c","active_low_in":true,"inputs":[{"pin":1,"dpt":"07fafa6a-b31a-431e-8510-09d5212ecefc"},{"pin":2,"dpt":"0dab65a4-92fa-420d-990c-eaf82da508ea"}],"outputs":[{"pin":0,"dpt":"7699a3b3-ebf5-444a-8cd8-49776dba5f5d"}]}
])");

const __FlashStringHelper* config_2 = FPSTR(R"([
{"type":"DigitalIn","pin":42,"input_type":"floating","active_low":true,"dpt":"3586608f-5d26-478a-abc2-223fd70664e8","uuid":"8ac7fcc4-e7ae-4bf4-a303-979b5fae3fa1"},
{"type":"DigitalIn","pin":41,"input_type":"floating","active_low":true,"dpt":"772b4ba0-e32e-48fa-ac28-2c464b4f25d2","uuid":"0c610345-fe55-4caa-ae63-231e0088060a"},
{"type":"DigitalIn","pin":40,"input_type":"floating","active_low":true,"dpt":"7a6ff5d9-b979-4318-b3af-5e76876d6ca7","uuid":"55d4b7b1-f8d9-4370-8c89-26db5209a113"},
{"type":"DigitalIn","pin":39,"input_type":"floating","active_low":true,"dpt":"070c5e6e-2487-41c8-93b5-be9adad42af5","uuid":"d213edba-4756-46ee-85c1-bbfd2121691b"},
{"type":"DigitalIn","pin":38,"input_type":"floating","active_low":true,"dpt":"a8274676-bf67-409b-a364-bddfeb18884a","uuid":"69df61ed-f528-4974-b084-d5b70be71d76"},
{"type":"DigitalIn","pin":48,"input_type":"floating","active_low":true,"dpt":"d21f228f-ef71-4d10-8ef6-d3b415e207e6","uuid":"7817677d-9e22-4721-b5ee-9b138103cbd7"},
{"type":"DigitalIn","pin":47,"input_type":"floating","active_low":true,"dpt":"4bc1b31d-3610-45bf-baf0-503afd45f6b5","uuid":"76581861-7c3b-4480-ab60-44682c8d606b"},
{"type":"DigitalIn","pin":21,"input_type":"floating","active_low":true,"dpt":"25a58394-1d7b-4645-8e08-a47952a49c1e","uuid":"90d8fd97-5cc7-4241-9c60-f6c0f6f79287"},
{"type":"DigitalIn","pin":6,"input_type":"floating","active_low":true,"dpt":"cd9f1cd2-0252-4c6e-b9eb-7868de72face","uuid":"6e3f58b2-89ba-4e4c-9a7a-8eaec27c0ed5"},
{"type":"DigitalOut","pin":46,"dpt":"68d4ce65-7c21-4ff3-be5e-160e7943573a","uuid":"3faabb21-e3f0-49bb-a84c-3e47a6e935fb"},
{"type":"DigitalOut","pin":3,"dpt":"68d4ce65-7c21-4ff3-be5e-160e7943573a","uuid":"4a6a9a0c-3af3-4c1c-84df-0f35c4ad58c4"},
{"speed": 800.0, "variant": "NeoPixel_RGB", "color_encoding": "grb", "led_pin": 1.0, "led_count": 1.0, "uuid": "2048bc79-2caf-4265-a713-4a5b5fa26394", "type": "NeoPixel", "version": 0}
])");

std::array<const __FlashStringHelper*, 2> configs{config_1, config_2};

void setRegisterFixedPeripherals(JsonObject msg) {}

#else

std::array<const __FlashStringHelper*, 2> configs{nullptr, nullptr};
void setRegisterFixedPeripherals(JsonObject msg) {}

#endif

}  // namespace fixed
}  // namespace peripheral
}  // namespace inamata