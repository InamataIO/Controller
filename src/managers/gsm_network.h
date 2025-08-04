#pragma once

#include "Arduino.h"
#include "chrono"

// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial
// Set serial for AT commands (to the module)
#define SerialAT Serial1

// Increase RX buffer to capture the entire response
// Chips without internal buffering (A6/A7, ESP8266, M590)
// need enough space in the buffer for the entire response
// else data will be lost (and the http library will fail).
#if !defined(TINY_GSM_RX_BUFFER)
#define TINY_GSM_RX_BUFFER 650
#endif

// See all AT commands, if wanted
#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
#endif

// Define the serial console for debug prints, if needed
#define TINY_GSM_DEBUG SerialMon

#include <TinyGsmClient.h>

namespace inamata {
class GsmNetwork {
 public:
  GsmNetwork();
  ~GsmNetwork() = default;

  /**
   * Blocking call to enable GSM modem
   */
  void enable();

  /**
   * Disable GSM modem
   */
  void disable();

  /**
   * Whether GSM network is enabled
   *
   * \note Check isNetworkConnected and isGprsConnected for connection state
   */
  bool isEnabled() const;

  /**
   * Start the time sync process
   */
  void syncTime();

  /**
   * Try to connect to GSM/EDGE/LTE network
   */
  void handleConnection();

  /**
   * True if data connection is up
   */
  bool isGprsConnected();

  /**
   * True if registered with the network (SMS / calls)
   */
  bool isNetworkConnected();

  /**
   * Encode string into SMS GSM-7 encoding
   *
   * \param text The text to encode
   * \return The encoded text
   */
  static String encodeSms(const char* text);

  bool network_connected_ = false;
  bool gprs_connected_ = false;
  int16_t signal_quality_ = 0;

  TinyGsm modem_;
  TinyGsmClient client_;

 private:
  bool is_enabled_ = false;
#ifdef DUMP_AT_COMMANDS
  StreamDebugger debugger_;
#endif

  std::chrono::seconds check_period_ = std::chrono::seconds(2);
  std::chrono::steady_clock::time_point last_network_check_ =
      std::chrono::steady_clock::time_point::min();
};
}  // namespace inamata
