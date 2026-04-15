#pragma once

#include <Arduino.h>

#include <chrono>
#include <memory>

#include "managers/storage.h"

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
  struct AtJob {
    bool active = false;
    uint32_t started_ms = 0;
    uint32_t timeout_ms = 180000;  // COPS=? takes ~30s, add more for safety
    String buffer;
    bool success = false;
  };

  enum class CopsScanType { kDefault, kAuto, kGsm, kLte };

  GsmNetwork(std::shared_ptr<Storage> storage);
  ~GsmNetwork() = default;

  static const char* type_;

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

  void startCopsScan(CopsScanType scan_type = CopsScanType::kDefault);
  void pollCopsScan();
  void clearCopsScan();

  /**
   * Clear and set allowed mobile operators
   *
   * Saves vector of MCC/MNC codes as JSON list of strings. Clears existing
   * operators. Format as 5-6 digit string per operator, no delimiter between
   * MCC and MNC code.
   *
   * \param mnos List of MCC/MNC mobile network operator codes
   * \return Details if an error occured
   */
  ErrorResult setAllowedMobileOperators(const std::vector<String>& mnos);

  std::unique_ptr<AtJob> cops_scan_;
  String cops_result_;

  bool network_connected_ = false;
  bool gprs_connected_ = false;
  int16_t signal_quality_ = 0;
  int16_t network_system_mode_ = 0;
  /// The MCC/MNC tuple of the currently connected Mobile Network Operator
  String current_mno_;

#ifdef DUMP_AT_COMMANDS
  StreamDebugger debugger_;
#endif
  TinyGsm modem_;
  TinyGsmClient client_;

 private:
  enum class ConnectionState {
    kIdle,
    kTryingCandidate,
    kConnected,
    kDisconnected,
  };

  /**
   * Attempt to connect to MNO in the allowed list
   *
   * Explicitly disconnects if the list is empty.
   */
  void enterModemConnectMode();

  /**
   * Starts scanning for available MNOs
   *
   * \param scan_type Which network technology to scan on (LTE/GSM)
   */
  void enterModemScanMode(CopsScanType scan_type);

  /**
   * Tries to connect to the specified mobile network operator
   *
   * \param mno MCC/MVC tuple of the operator
   */
  void connectToMno(const String& mno);

  /**
   * Disconnects the modem
   */
  void disconnectModem();

  /**
   * Returns the next allowed MNO code, wraps after last item.
   *
   * \return Next MNO code or blank String on empty list
   */
  String iterateNextAllowedMno();

  ErrorResult validateAllowedMobileOperators(const std::vector<String>& mnos);

  void saveLastConnectedMno(const String& mno);

  std::shared_ptr<Storage> storage_;

  bool is_enabled_ = false;

  ConnectionState connection_state_ = ConnectionState::kIdle;
  /// List of Mobile Network Operators it's allowed to connect to (MCC/MNC)
  std::vector<String> allowed_mnos_;
  /// Index of the allowed_mnos_ that will be used on next connect attempt
  size_t next_mno_idx_ = 0;
  /// Target MNO code to connect to
  String connect_to_mno_;
  /// When the last attempt to connect to a network was started
  std::chrono::steady_clock::time_point connection_attempt_start_ =
      std::chrono::steady_clock::time_point::min();
  std::chrono::seconds max_connection_period_ = std::chrono::seconds(60);

  std::chrono::seconds check_period_ = std::chrono::seconds(10);
  std::chrono::steady_clock::time_point last_network_check_ =
      std::chrono::steady_clock::time_point::min();

  /// Key to list of allowed Mobile Network Operator MCC/MVC tuple
  static const char* allowed_mnos_key_;
  /// Key to MCC/MVC code of last connected MNO
  static const char* last_connected_mno_key_;
};
}  // namespace inamata
