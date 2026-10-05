#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <utils/uuid.h>

namespace inamata {
namespace peripheral {
namespace fixed {

extern std::array<const char*, 2> configs;
extern const char* config_path;
void setRegisterFixedPeripherals(JsonObject msg);

// Common data point type IDs
#ifdef FIXED_PERIPHERALS_ACTIVE

/// Bool state of a button
extern const char* dpt_button_id;
/// Bool state of a buzzer
extern const char* dpt_buzzer_id;
/// Air CO2 concentration in PPM
extern const char* dpt_co2_ppm_id;
/// Electric current in amps
extern const char* dpt_current_a_id;
/// Bool state of a heater
extern const char* dpt_heater_id;
/// Air humidity in RH %
extern const char* dpt_humidity_rh_id;
/// Bool state of an LED
extern const char* dpt_led_id;
/// Bool state of a relay
extern const char* dpt_relay_id;
/// Air temperature in °C
extern const char* dpt_temperature_c_id;
/// Air VOC quality in index (0-500, 100=baseline)
extern const char* dpt_voc_index_id;
/// Air TVOC concentration in PPM
extern const char* dpt_voc_ppm_id;
/// Air formaldehyde (HCHO) concentration in ug/m3
extern const char* dpt_formaldehyde_id;
/// Air NO2 concentration in ppm
extern const char* dpt_no2_id;
/// Air NH3 concentration in ppm
extern const char* dpt_nh3_id;
/// Air SO2 concentration in ppm
extern const char* dpt_so2_id;
/// Air PM2.5 concentration in ppm
extern const char* dpt_pm2_5_id;
/// Air CO concentration in ppm
extern const char* dpt_co_id;
/// Air Carbon Dioxide / CO2 concentration in ppm
extern const char* dpt_co2_id;
/// Air Oxygen / O2 concentration in ppm
extern const char* dpt_o2_id;
/// Air Ozone / O3 concentration in ppm
extern const char* dpt_o3_id;

#endif

#ifdef DEVICE_TYPE_VOC_SENSOR_MK1

extern const char* peripheral_voc_id;
extern const char* peripheral_air_id;
extern const char* peripheral_status_led_id;

#elif defined(DEVICE_TYPE_TIAKI_CO2_MONITOR)

extern const char* peripheral_led_alarm_1_id;
extern const char* peripheral_led_alarm_2_id;
extern const char* peripheral_led_alarm_3_id;
extern const char* peripheral_led_alarm_4_id;
extern const char* peripheral_led_fault_id;
extern const char* peripheral_led_network_id;

extern const char* peripheral_touch_1_id;
extern const char* peripheral_touch_2_id;

extern const char* peripheral_buzzer_id;

extern const char* peripheral_relay_1_id;
extern const char* peripheral_relay_2_id;
extern const char* peripheral_relay_3_id;
extern const char* peripheral_relay_4_id;

extern const char* peripheral_i2c_adapter_id;
extern const char* peripheral_analog_out_id;

extern const char* peripheral_modbus_client_id;
extern const char* peripheral_modbus_sensor_in_id;
extern const char* peripheral_modbus_sensor_out_id;

#elif defined(DEVICE_TYPE_FIRE_DATA_LOGGER)

extern const utils::UUID peripheral_i2c_adapter_id;
extern const utils::UUID peripheral_io_1_id;
extern const utils::UUID peripheral_io_2_id;

extern const utils::UUID peripheral_electric_control_circuit_fail_id;

extern const utils::UUID peripheral_jockey_1_pump_run_id;
extern const utils::UUID peripheral_jockey_2_pump_run_id;
extern const utils::UUID peripheral_jockey_1_pump_fail_id;
extern const utils::UUID peripheral_jockey_2_pump_fail_id;

extern const utils::UUID peripheral_pumphouse_protection_alarm_id;
extern const utils::UUID peripheral_annunciator_fault_id;
extern const utils::UUID peripheral_pumphouse_flooding_alarm_id;
extern const utils::UUID peripheral_maintenance_input_id;

extern const utils::UUID peripheral_relay_1_id;
extern const utils::UUID peripheral_relay_2_id;

extern const utils::UUID peripheral_heartbeat_led_id;
extern const utils::UUID peripheral_gsm_wifi_toggle_id;
extern const utils::UUID peripheral_maintenance_button_id;

extern const utils::UUID peripheral_status_led_id;

extern const utils::UUID dpt_diesel_1_fire_alarm_id;
extern const utils::UUID dpt_diesel_2_fire_alarm_id;
extern const utils::UUID dpt_diesel_3_fire_alarm_id;
extern const utils::UUID dpt_diesel_4_fire_alarm_id;

extern const utils::UUID dpt_diesel_1_pump_run_id;
extern const utils::UUID dpt_diesel_2_pump_run_id;
extern const utils::UUID dpt_diesel_3_pump_run_id;
extern const utils::UUID dpt_diesel_4_pump_run_id;

extern const utils::UUID dpt_diesel_1_pump_fail_id;
extern const utils::UUID dpt_diesel_2_pump_fail_id;
extern const utils::UUID dpt_diesel_3_pump_fail_id;
extern const utils::UUID dpt_diesel_4_pump_fail_id;

extern const utils::UUID dpt_diesel_1_battery_charger_fail_id;
extern const utils::UUID dpt_diesel_2_battery_charger_fail_id;
extern const utils::UUID dpt_diesel_3_battery_charger_fail_id;
extern const utils::UUID dpt_diesel_4_battery_charger_fail_id;

extern const utils::UUID dpt_diesel_1_low_oil_level_fail_id;
extern const utils::UUID dpt_diesel_2_low_oil_level_fail_id;
extern const utils::UUID dpt_diesel_3_low_oil_level_fail_id;
extern const utils::UUID dpt_diesel_4_low_oil_level_fail_id;

extern const utils::UUID dpt_diesel_control_circuit_fail_id;
extern const utils::UUID dpt_diesel_mains_fail_id;
extern const utils::UUID dpt_diesel_pump_fail_id;
extern const utils::UUID dpt_diesel_engine_overheat_fail_id;
extern const utils::UUID dpt_diesel_fuel_tank_low_id;

extern const utils::UUID dpt_electric_1_fire_alarm_id;
extern const utils::UUID dpt_electric_2_fire_alarm_id;
extern const utils::UUID dpt_electric_1_pump_run_id;
extern const utils::UUID dpt_electric_2_pump_run_id;
extern const utils::UUID dpt_electric_1_pump_fail_id;
extern const utils::UUID dpt_electric_2_pump_fail_id;
extern const utils::UUID dpt_electric_mains_fail_id;
extern const utils::UUID dpt_electric_control_circuit_fail_id;

extern const utils::UUID dpt_jockey_1_pump_run_id;
extern const utils::UUID dpt_jockey_2_pump_run_id;
extern const utils::UUID dpt_jockey_1_pump_fail_id;
extern const utils::UUID dpt_jockey_2_pump_fail_id;
extern const utils::UUID dpt_pumphouse_protection_alarm_id;
extern const utils::UUID dpt_annunciator_fault_id;
extern const utils::UUID dpt_pumphouse_flooding_alarm_id;
extern const utils::UUID dpt_maintenance_input_id;

extern const utils::UUID dpt_gsm_wifi_toggle_id;
extern const utils::UUID dpt_maintenance_mode_id;

extern const uint8_t gsm_enable_pin;
extern const uint8_t gsm_reset_pin;
extern const uint8_t gsm_tx_pin;
extern const uint8_t gsm_rx_pin;

#elif defined(DEVICE_TYPE_MULTI_AIR_SENSOR)

extern const uint8_t gsm_enable_pin;
extern const uint8_t gsm_reset_pin;
extern const uint8_t gsm_tx_pin;
extern const uint8_t gsm_rx_pin;

#endif

}  // namespace fixed
}  // namespace peripheral
}  // namespace inamata