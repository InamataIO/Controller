#pragma once

#include <ArduinoJson.h>

#include <chrono>
#include <memory>

#include "managers/service_getters.h"
#include "peripheral/capabilities/get_values.h"
#include "peripheral/capabilities/start_measurement.h"
#include "peripheral/peripheral.h"
#include "utils/uuid.h"

namespace inamata {
namespace peripheral {
namespace peripherals {
namespace fs00603 {

/**
 * Driver for the FS00603 air quality sensor (CO2, TVOC, formaldehyde)
 *
 * Continuously streams 9-byte frames (0xFF header + 6 data bytes + checksum)
 * at 9600 8N1. Buffers a frame via the startMeasurement/handleMeasurement
 * interface and reports the last valid frame's values via getValues.
 *
 * A completed measurement is cached for one sensor update interval. Multiple
 * tasks sharing this peripheral therefore receive the same reading
 * immediately, while only one task polls for the next frame.
 */
class FS00603 : public Peripheral,
                public capabilities::GetValues,
                public capabilities::StartMeasurement {
 public:
  FS00603(const JsonObjectConst& parameters);
  virtual ~FS00603() = default;

  const String& getType() const final;
  static const String& type();

  capabilities::StartMeasurement::Result startMeasurement(
      const JsonVariantConst& parameters) final;
  capabilities::StartMeasurement::Result handleMeasurement() final;
  capabilities::GetValues::Result getValues() final;

 private:
  static std::shared_ptr<Peripheral> factory(const ServiceGetters& services,
                                             const JsonObjectConst& parameters);

  capabilities::StartMeasurement::Result readFrame();
  void resetState();
  void onUartError(hardwareSerial_error_t error);
  bool hasCachedMeasurement() const;
  static uint8_t calculateChecksum(const uint8_t* data);

  static bool registered_;
  static bool capability_get_values_;
  static bool capability_start_measurement_;

  static const __FlashStringHelper* co2_data_point_type_key_;
  static const __FlashStringHelper* voc_data_point_type_key_;
  static const __FlashStringHelper* formaldehyde_data_point_type_key_;

  HardwareSerial* serial_ = nullptr;
  utils::UUID co2_data_point_type_{nullptr};
  utils::UUID voc_data_point_type_{nullptr};
  utils::UUID formaldehyde_data_point_type_{nullptr};

  // Set from the UART event task on a break condition; checked/cleared on
  // the polling task so the in-progress frame (corrupt around the break) is
  // discarded instead of waiting on a checksum failure to notice
  volatile bool uart_break_detected_ = false;

  static constexpr uint8_t frame_size_ = 9;
  static constexpr uint8_t frame_header_ = 0xFF;
  uint8_t in_data_[frame_size_]{};
  uint8_t in_data_i_ = 0;

  float last_co2_ppm_ = NAN;
  float last_voc_ppm_ = NAN;
  float last_formaldehyde_ugm3_ = NAN;
  bool started_ = false;
  bool measurement_ready_ = false;
  std::chrono::steady_clock::time_point measurement_ready_time_{};

  std::chrono::steady_clock::time_point measurement_start_;
  static constexpr std::chrono::seconds measurement_timeout_{10};
  static constexpr std::chrono::milliseconds no_data_wait_{500};
  static constexpr std::chrono::milliseconds kInitialWait{5000};
  static constexpr std::chrono::milliseconds kCacheDuration{2000};
};

}  // namespace fs00603
}  // namespace peripherals
}  // namespace peripheral
}  // namespace inamata
