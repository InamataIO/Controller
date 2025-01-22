#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

namespace inamata {
namespace peripheral {
namespace fixed {

extern std::array<const __FlashStringHelper*, 2> configs;
void setRegisterFixedPeripherals(JsonObject msg);

// Common data point type IDs
#ifdef FIXED_PERIPHERALS_ACTIVE

/// Bool state of a button
extern const __FlashStringHelper* dpt_button_id;
/// Bool state of a buzzer
extern const __FlashStringHelper* dpt_buzzer_id;
/// Air CO2 concentration in PPM
extern const __FlashStringHelper* dpt_co2_ppm_id;
/// Electric current in amps
extern const __FlashStringHelper* dpt_current_a_id;
/// Bool state of a heater
extern const __FlashStringHelper* dpt_heater_id;
/// Air humidity in RH %
extern const __FlashStringHelper* dpt_humidity_rh_id;
/// Bool state of an LED
extern const __FlashStringHelper* dpt_led_id;
/// Bool state of a relay
extern const __FlashStringHelper* dpt_relay_id;
/// Air temperature in °C
extern const __FlashStringHelper* dpt_temperature_c_id;
/// Air VOC quality in index (0-500, 100=baseline)
extern const __FlashStringHelper* dpt_voc_index_id;

#endif

#ifdef DEVICE_TYPE_VOC_SENSOR_MK1

extern const __FlashStringHelper* peripheral_voc_id;
extern const __FlashStringHelper* peripheral_air_id;
extern const __FlashStringHelper* peripheral_status_led_id;

#elif defined(DEVICE_TYPE_TIAKI_CO2_MONITOR)

extern const __FlashStringHelper* peripheral_led_alarm_1_id;
extern const __FlashStringHelper* peripheral_led_alarm_2_id;
extern const __FlashStringHelper* peripheral_led_alarm_3_id;
extern const __FlashStringHelper* peripheral_led_alarm_4_id;
extern const __FlashStringHelper* peripheral_led_fault_id;
extern const __FlashStringHelper* peripheral_led_network_id;

extern const __FlashStringHelper* peripheral_touch_1_id;
extern const __FlashStringHelper* peripheral_touch_2_id;

extern const __FlashStringHelper* peripheral_buzzer_id;

extern const __FlashStringHelper* peripheral_relay_1_id;
extern const __FlashStringHelper* peripheral_relay_2_id;
extern const __FlashStringHelper* peripheral_relay_3_id;
extern const __FlashStringHelper* peripheral_relay_4_id;

extern const __FlashStringHelper* peripheral_i2c_adapter_id;
extern const __FlashStringHelper* peripheral_analog_out_id;

extern const __FlashStringHelper* peripheral_modbus_client_id;
extern const __FlashStringHelper* peripheral_modbus_sensor_in_id;
extern const __FlashStringHelper* peripheral_modbus_sensor_out_id;

#endif

}  // namespace fixed
}  // namespace peripheral
}  // namespace inamata