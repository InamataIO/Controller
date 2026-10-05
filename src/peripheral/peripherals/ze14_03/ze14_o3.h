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
namespace ze14_03 {

class ZE14_O3 : public Peripheral,
                public capabilities::GetValues,
                public capabilities::StartMeasurement {
 public:
  ZE14_O3(const JsonObjectConst& parameters);
  virtual ~ZE14_O3() = default;

  const String& getType() const final;
  static const String& type();

  capabilities::StartMeasurement::Result startMeasurement(
      const JsonVariantConst& parameters) final;
  capabilities::StartMeasurement::Result handleMeasurement() final;
  capabilities::GetValues::Result getValues() final;

 private:
  static std::shared_ptr<Peripheral> factory(const ServiceGetters& services,
                                             const JsonObjectConst& parameters);

  void issueRequest();
  void writeCommand();
  bool readResponse();

  static bool registered_;
  static bool capability_get_values_;
  static bool capability_start_measurement_;

  HardwareSerial* serial_ = nullptr;
  utils::UUID data_point_type_{nullptr};

  enum class State {
    kIdle,     // No request has been made (yet), or the last one timed out
    kWaiting,  // Request sent, waiting on the response
    kReady,    // Response parsed, last_ozone_ppb_ is valid
  };
  State state_ = State::kIdle;

  // When the current/last request was sent; only used for the timeout
  std::chrono::steady_clock::time_point request_time_;
  // When the cached reading became valid; used to judge cache staleness
  std::chrono::steady_clock::time_point ready_time_;
  float last_ozone_ppb_ = NAN;

  uint8_t response_[9]{};
  uint8_t response_i_ = 0;

  static constexpr std::chrono::milliseconds measurement_wait_{100};
  static constexpr std::chrono::milliseconds measurement_timeout_{1000};
  // Multiple tasks poll this peripheral independently; cache a reading for
  // this long so a faster consumer doesn't force a re-request that would
  // disrupt a slower consumer still reading the previous response
  static constexpr std::chrono::milliseconds min_measurement_interval_{3000};
};

}  // namespace ze14_03
}  // namespace peripherals
}  // namespace peripheral
}  // namespace inamata
