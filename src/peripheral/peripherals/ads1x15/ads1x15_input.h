#pragma once

#include "peripheral/capabilities/get_values.h"
#include "peripheral/capabilities/start_measurement.h"
#include "peripheral/peripherals/ads1x15/ads1x15_adapter.h"
#include "peripheral/peripherals/analog.h"

namespace inamata {
namespace peripheral {
namespace peripherals {
namespace ads1x15 {

class ADS1X15Input : public Peripheral,
                     public capabilities::GetValues,
                     public capabilities::StartMeasurement,
                     private Analog {
 public:
  ADS1X15Input(const JsonObjectConst& parameters);
  virtual ~ADS1X15Input() = default;

  // Type registration in the peripheral factory
  const String& getType() const final;
  static const String& type();

  /**
   * Start an ADC measurement
   *
   * \param parameters
   * \return The time until the result is ready to be read (~8 ms)
   */
  capabilities::StartMeasurement::Result startMeasurement(
      const JsonVariantConst& parameters) final;

  /**
   * Check the measurement state
   *
   * If the measurement is not stable yet, it returns a wait time until the
   * next reading has been taken.
   *
   * \return The time wait, if an error occured or if the measurement completed
   */
  capabilities::StartMeasurement::Result handleMeasurement() final;

  /**
   * Reads all available data points from the ADS1X15Adapter
   *
   * \return A vector with all read data points and their type
   */
  capabilities::GetValues::Result getValues() final;

 private:
  static std::shared_ptr<Peripheral> factory(const ServiceGetters& services,
                                             const JsonObjectConst& parameter);
  static bool registered_;
  static bool capability_get_values_;

  uint16_t channel_;

  std::shared_ptr<ADS1X15Adapter> ads1x15_adapter_;

  adsGain_t gain_ = GAIN_TWO;
  static const __FlashStringHelper* gain_key_;
  static const __FlashStringHelper* gain_key_error_;

  static const __FlashStringHelper* ads1x15_adapter_key_;
  static const __FlashStringHelper* ads1x15_adapter_key_error_;
  static const __FlashStringHelper* channel_key_;
  static const __FlashStringHelper* channel_key_error_;
};

}  // namespace ads1x15
}  // namespace peripherals
}  // namespace peripheral
}  // namespace inamata