#pragma once

#include <Adafruit_ADS1X15.h>
#include <ArduinoJson.h>

#include <deque>

#include "peripheral/capabilities/start_measurement.h"
#include "peripheral/peripheral.h"
#include "peripheral/peripherals/i2c/i2c_abstract_peripheral.h"

namespace inamata {
namespace peripheral {
namespace peripherals {
namespace ads1x15 {

class ADS1X15Adapter : public peripherals::i2c::I2CAbstractPeripheral {
 public:
  struct ReadingConfig {
    /// The mux value from Adafruit_ADS1X15.h
    uint16_t channel;
    adsGain_t gain;
    void* caller;
    // Used to filter out readings that were not properly closed
    std::chrono::steady_clock::time_point started_at;
  };

  struct Result {
    float voltage;
    ErrorResult error;
  };

  ADS1X15Adapter(const JsonObjectConst& parameters);
  virtual ~ADS1X15Adapter() = default;

  // Type registration in the peripheral factory
  const String& getType() const final;
  static const String& type();

  /**
   * Start an ADC reading for the channel and gain. Finished set true on ready
   *
   * \param channel A single ended or differential channel
   * \param gain The gain to be used x2/3 to x16
   * \param caller Pointer identifying the caller
   * \return How long to wait for and if an error occured
   */
  capabilities::StartMeasurement::Result startADCReading(
      ReadingConfig reading_config);

  /**
   * Checks if the ADC reading for the caller is finished
   *
   * \param caller Pointer identifying the caller
   * \return 0 if finished, else time to wait, -1 if not queued
   */
  std::chrono::nanoseconds isFinished(void* caller);

  /**
   * Gets the value of the last ADC reading
   *
   * \return ADC reading value in volts. NAN if startADCReading was not called
   */
  Result completeReading(void* caller);

 private:
  /**
   * Start the next reading if any in the queued readings queue
   */
  void startNextReading();

  static std::shared_ptr<Peripheral> factory(const ServiceGetters& services,
                                             const JsonObjectConst& parameters);
  static bool registered_;

  /// The driver to read data from the ADS1115 / ADS1015
  std::unique_ptr<Adafruit_ADS1X15> driver_;
  /// ADS1X15 I2C address
  uint8_t i2c_address_;

  /// When starting readings, how long to wait for
  const std::chrono::milliseconds default_wait_time_{10};
  /// List of all queued readings
  std::deque<ReadingConfig> queued_readings_;
  /// The error if the chip type does not match the expected values
  static const __FlashStringHelper* invalid_chip_type_error_;
  /// If it is an ADS1015 or ADS1115
};

}  // namespace ads1x15
}  // namespace peripherals
}  // namespace peripheral
}  // namespace inamata