#ifdef GSM_NETWORK

#include "gsm_network.h"

#include "configuration.h"
#include "logging.h"
#include "managers/services.h"
#include "peripheral/fixed.h"
#include "utils/chrono.h"

#ifdef RTC_MANAGER
#include "managers/time_manager.h"
#endif

namespace inamata {

GsmNetwork::GsmNetwork(std::shared_ptr<Storage> storage)
    :
#ifdef DUMP_AT_COMMANDS
      debugger_(SerialAT, SerialMon),
      modem_(debugger_),
#else
      modem_(SerialAT),
#endif
      client_(modem_),
      storage_(storage) {
  pinMode(peripheral::fixed::gsm_enable_pin, OUTPUT);
  pinMode(peripheral::fixed::gsm_reset_pin, OUTPUT);
  SerialAT.begin(115200, SERIAL_8N1, peripheral::fixed::gsm_rx_pin,
                 peripheral::fixed::gsm_tx_pin);

  // Load config from file. Get allowed MNOs and set last connected as default
  JsonDocument mobile_config_doc;
  ErrorResult result = storage_->loadMobileConfig(mobile_config_doc);
  if (result.isError()) {
    TRACELN(result.toString());
    return;
  }
  JsonObject mobile_config = mobile_config_doc.as<JsonObject>();
  JsonArray allowed_mnos = mobile_config[allowed_mnos_key_].as<JsonArray>();
  const char* last_connected_mno =
      mobile_config[last_connected_mno_key_].as<const char*>();

  // Restore allowed MNOs. Connect to last connected MNO if in allow list
  for (size_t i = 0; i < allowed_mnos.size(); i++) {
    JsonVariantConst mno = allowed_mnos[i];
    allowed_mnos_.push_back(mno);
    if (mno == last_connected_mno) {
      connect_to_mno_ = last_connected_mno;
      next_mno_idx_ = i + 1;
    }
  }
}

void GsmNetwork::enable() {
  digitalWrite(peripheral::fixed::gsm_enable_pin, HIGH);
  delay(200);
  digitalWrite(peripheral::fixed::gsm_reset_pin, LOW);
  delay(200);
  digitalWrite(peripheral::fixed::gsm_reset_pin, HIGH);
  delay(2000);
  modem_.init();
  connection_state_ = ConnectionState::kIdle;
  enterModemConnectMode();

  is_enabled_ = true;
}

void GsmNetwork::disable() {
  digitalWrite(peripheral::fixed::gsm_enable_pin, LOW);

  disconnectModem();
  is_enabled_ = false;
}

bool GsmNetwork::isEnabled() const { return is_enabled_; }

void GsmNetwork::syncTime() {
  modem_.sendAT(GF("+CNTP=\"pool.ntp.org\",0"));
  modem_.waitResponse();

  modem_.sendAT(GF("+CNTP"));
  modem_.waitResponse(3000, GF("+CNTP: 0" AT_NL));

  modem_.sendAT(GF("+CCLK?"));
  modem_.waitResponse(GF(AT_NL));
  String cclk = modem_.stream.readStringUntil('\n');
  modem_.waitResponse();
  String datetime =
      cclk.substring(cclk.indexOf("\"") + 1, cclk.lastIndexOf("\""));
  datetime.trim();

  int day, month, year, hour, minute, second, tz_offset = 0;
  char tz_sign = 0;  // To store '+' or '-' if present

  // Try parsing with timezone offset
  int result =
      sscanf(datetime.c_str(), "%2d/%2d/%2d,%2d:%2d:%2d%c%2d", &day, &month,
             &year, &hour, &minute, &second, &tz_sign, &tz_offset);

  // If timezone offset was not found, try parsing without it
  if (result < 7) {
    result = sscanf(datetime.c_str(), "%2d/%2d/%2d,%2d:%2d:%2d", &day, &month,
                    &year, &hour, &minute, &second);
    tz_offset = 0;  // No timezone offset
    tz_sign = 0;
  }

  // Adjust timezone offset based on sign
  if (tz_sign == '-') {
    tz_offset = -tz_offset;
  }

  TRACEF("DT: 20%02d-%02d-%02dT%02d:%02d:%02d%c%02d:%02d\r\n", year, month, day,
         hour, minute, second, tz_sign, std::abs(tz_offset) / 4,
         (std::abs(tz_offset) % 4) * 15);
  if (result < 6) {
    Serial.println("GSM NTP sync failed");
  }

  struct tm time_struct = {0};
  time_t epoch = 0;
  time_struct.tm_mday = day;       // Day of the month (1-31)
  time_struct.tm_mon = month - 1;  // Month (0-11, so subtract 1)
  time_struct.tm_year =
      year + 100;               // Year since 1900 (assuming 2013, not 1913)
  time_struct.tm_hour = hour;   // Hour (0-23)
  time_struct.tm_min = minute;  // Minute (0-59)
  time_struct.tm_sec = second;  // Second (0-59)
  time_struct.tm_isdst = -1;    // Let system determine daylight saving time

  epoch = mktime(&time_struct);
  struct timeval tv;
  tv.tv_sec = epoch;
  tv.tv_usec = 0;
  settimeofday(&tv, NULL);

  Services::is_time_synced_ = true;

#ifdef RTC_MANAGER
  if (TimeManager::lostPower()) {
    TimeManager::setSystemTime(DateTime(epoch));
    TRACELN("GSM updated RTC clock");
  }
#endif
}

void GsmNetwork::handleConnection() {
  if (cops_scan_ && cops_scan_->active) {
    pollCopsScan();
    return;
  }

  const auto now = std::chrono::steady_clock::now();
  if (utils::chrono_abs(now - last_network_check_) > check_period_) {
    last_network_check_ = now;

    network_connected_ = modem_.isNetworkConnected();
    if (network_connected_) {
      gprs_connected_ = modem_.isGprsConnected();
      signal_quality_ = modem_.getSignalQuality();
      current_mno_ = modem_.getOperator();
      // Get NSM (GPRS, EDGE, LTE) and ignore auto_nsm state
      bool auto_nsm;
      modem_.getNetworkSystemMode(auto_nsm, network_system_mode_);
      if (gprs_connected_) {
        connection_state_ = ConnectionState::kConnected;
      } else {
        gprs_connected_ = modem_.gprsConnect(kGsmApn);
      }
    } else {
      gprs_connected_ = false;
      signal_quality_ = 0;
      current_mno_ = "";
    }
    TRACEF("Network: %d GPRS: %d CSQ: %d, MNO: %s State: %d\r\n",
           network_connected_, gprs_connected_, signal_quality_,
           current_mno_.c_str(), connection_state_);

    // If idle connect to last connected MNO (loaded from LittleFS), else
    // if trying connect and timed out, iterate to next MNO
    if (connection_state_ == ConnectionState::kIdle) {
      if (!connect_to_mno_.length()) {
        connect_to_mno_ = iterateNextAllowedMno();
      }
      connectToMno(connect_to_mno_);
    } else if (connection_state_ == ConnectionState::kTryingCandidate &&
               utils::chrono_abs(now - connection_attempt_start_) >
                   max_connection_period_) {
      connect_to_mno_ = iterateNextAllowedMno();
      connectToMno(connect_to_mno_);
    }
  }
}

bool GsmNetwork::isNetworkConnected() { return network_connected_; }

bool GsmNetwork::isGprsConnected() { return gprs_connected_; }

String GsmNetwork::encodeSms(const char* text) {
  const size_t text_length = strlen(text);
  String gsm7;
  gsm7.reserve(text_length);
  for (size_t i = 0; i < text_length; i++) {
    char c = *(text + i);
    // Skip sections with matching encoding
    if ((c >= '%' && c <= '?') || (c >= 'A' && c <= 'Z') ||
        (c >= 'a' && c <= 'z') || (c >= ' ' && c <= '#') || c == '\n' ||
        c == '\r') {
      gsm7 += c;
      continue;
    }
    // Handle mapping
    if (c == '@') {
      c = 0x00;
    } else if (c == '$') {
      c = 0x02;
    } else if (c == '[') {
      c = 0x28;
    } else if (c == '\\') {
      c = 0x2F;
    } else if (c == ']') {
      c = 0x29;
    } else if (c == '_') {
      c = 0x11;
    } else if (c == '`') {
      c = 0x27;
    } else if (c == '{') {
      c = 0x28;
    } else if (c == '|') {
      c = 'I';
    } else if (c == '}') {
      c = 0x29;
    } else if (c == '~') {
      c = 0x2D;
    } else {
      c = 0x11;
    }
    gsm7 += c;
  }
  return gsm7;
}

void GsmNetwork::startCopsScan(CopsScanType scan_type) {
  if (cops_scan_ && cops_scan_->active) {
    return;
  }

  cops_scan_ = std::make_unique<AtJob>();
  cops_scan_->active = true;
  cops_scan_->started_ms = millis();
  cops_result_ = "";
  enterModemScanMode(scan_type);
}

void GsmNetwork::pollCopsScan() {
  if (!cops_scan_ || !cops_scan_->active) {
    return;
  }

  if (millis() - cops_scan_->started_ms > cops_scan_->timeout_ms) {
    cops_scan_->success = false;
    cops_scan_->active = false;

    enterModemConnectMode();
    return;
  }

  while (modem_.stream.available()) {
    char c = (char)modem_.stream.read();
    if (c == '\r') continue;

    if (c == '\n') {
      // We have a full line in currentLine
      if (cops_scan_->buffer == "OK") {
        cops_scan_->success = true;
        cops_scan_->active = false;
        cops_scan_->buffer = "";
        enterModemConnectMode();
        return;
      }
      if (cops_scan_->buffer == "ERROR" ||
          cops_scan_->buffer.startsWith("+CME ERROR:")) {
        cops_scan_->success = false;
        cops_scan_->active = false;
        cops_scan_->buffer = "";
        enterModemConnectMode();
        return;
      }

      // Capture the network list line
      if (cops_scan_->buffer.startsWith("+COPS:")) {
        cops_result_ = cops_scan_->buffer.substring(6);
        TRACEF("COPS result: %s\r\n", cops_result_.c_str());
      } else {
        TRACEF("URC: %s\r\n", cops_scan_->buffer);
      }

      cops_scan_->buffer = "";
    } else {
      cops_scan_->buffer += c;
      if (cops_scan_->buffer.length() > 1024) {
        // prevent runaway
        cops_scan_->buffer.remove(0, cops_scan_->buffer.length() - 1024);
      }
    }
  }
}

void GsmNetwork::clearCopsScan() {
  cops_scan_ = nullptr;
  cops_result_ = "";
}

ErrorResult GsmNetwork::setAllowedMobileOperators(
    const std::vector<String>& mnos) {
  ErrorResult result = validateAllowedMobileOperators(mnos);
  if (result.isError()) {
    return result;
  }

  // Save the MNOs to LittleFS and the GsmNetwork object
  JsonDocument mobile_config_doc;
  result = storage_->loadMobileConfig(mobile_config_doc);
  if (result.isError()) {
    return result;
  }

  JsonObject mobile_config = mobile_config_doc.isNull()
                                 ? mobile_config_doc.to<JsonObject>()
                                 : mobile_config_doc.as<JsonObject>();

  // .to() creates new or clears existing array
  JsonArray allowed_mnos = mobile_config[allowed_mnos_key_].to<JsonArray>();
  allowed_mnos_.clear();
  for (const String& mno : mnos) {
    allowed_mnos.add(mno);
    allowed_mnos_.push_back(mno);
  }
  next_mno_idx_ = 0;

  result = storage_->storeMobileConfig(mobile_config);
  if (result.isError()) {
    return result;
  }

  // Apply new allowlist by triggering a fresh candidate selection cycle.
  connect_to_mno_ = "";
  if (is_enabled_) {
    connection_state_ = ConnectionState::kIdle;
    connection_attempt_start_ = std::chrono::steady_clock::time_point::min();
  }

  return ErrorResult();
}

const bool GsmNetwork::hasAllowedMobileOperators() const {
  return allowed_mnos_.size();
}

void GsmNetwork::enterModemScanMode(CopsScanType scan_type) {
  if (connection_state_ == ConnectionState::kDisconnected) {
    modem_.init();
    connection_state_ = ConnectionState::kIdle;
  }
  modem_.gprsDisconnect();
  modem_.sendAT("+COPS=0");
  modem_.waitResponse();
  // Set modem to LTE mode (crashes on auto mode: 2)
  switch (scan_type) {
    case CopsScanType::kAuto:
      modem_.sendAT("+CNMP=2");
      break;
    case CopsScanType::kGsm:
      modem_.sendAT("+CNMP=13");
      break;
    case CopsScanType::kDefault:
    case CopsScanType::kLte:
    default:
      modem_.sendAT("+CNMP=38");
      break;
  }
  modem_.waitResponse();
  // Scan for networks but don't wait for response
  modem_.sendAT("+COPS=?");
}

void GsmNetwork::enterModemConnectMode() {
  if (connection_state_ == ConnectionState::kDisconnected) {
    modem_.init();
    connection_state_ = ConnectionState::kIdle;
  }
  // Set modem to auto tech mode (LTE + GSM)
  modem_.sendAT("+CNMP=2");
  modem_.waitResponse();

  // Select a Mobile Operator to connect to
  // 1. Try last used operator
  // 2. Get next valid operator if empty
  // 3. Set disconnected mode if no valid options available
  if (!connect_to_mno_.length()) {
    connect_to_mno_ = iterateNextAllowedMno();
  }
  connectToMno(connect_to_mno_);
}

void GsmNetwork::connectToMno(const String& mno) {
  if (!mno.length()) {
    return disconnectModem();
  }
  if (connection_state_ == ConnectionState::kDisconnected) {
    modem_.init();
    connection_state_ = ConnectionState::kIdle;
  }
  if (mno == "*") {
    // Allow all MNOs. Allow modem to select best connection
    modem_.sendAT("+COPS=0");
    modem_.waitResponse();
  } else {
    // Only connect to specified modem
    modem_.sendAT("+COPS=1,2,\"" + mno + "\"");
    modem_.waitResponse();
  }
  // Save MNO connection attempt. Allows to iterate through long list of MNOs
  saveLastConnectedMno(connect_to_mno_);
  connection_state_ = ConnectionState::kTryingCandidate;
  connection_attempt_start_ = std::chrono::steady_clock::now();
  TRACEF("Trying to connect to: %s\r\n", mno.c_str());
}

void GsmNetwork::disconnectModem() {
  modem_.sendAT("+COPS=2");
  modem_.waitResponse();
  connection_state_ = ConnectionState::kDisconnected;
  connection_attempt_start_ = std::chrono::steady_clock::time_point::min();
  TRACELN("Disconnecting");
}

String GsmNetwork::iterateNextAllowedMno() {
  if (!allowed_mnos_.size()) {
    return String();
  }

  if (next_mno_idx_ >= allowed_mnos_.size()) {
    next_mno_idx_ = 0;
  }
  const String& mno = allowed_mnos_[next_mno_idx_];
  next_mno_idx_++;

  return mno;
}

ErrorResult GsmNetwork::validateAllowedMobileOperators(
    const std::vector<String>& mnos) {
  const char* error = "Invalid allowed MNO format";
  bool has_wildcard = false;
  for (const String& mno : mnos) {
    if (mno == "*") {
      has_wildcard = true;
      continue;
    }

    if (mno.length() != 5 && mno.length() != 6) {
      return ErrorResult(type_, error);
    }

    for (size_t i = 0; i < mno.length(); ++i) {
      if (!isDigit(mno[i])) {
        return ErrorResult(type_, error);
      }
    }
  }

  // "*" must be exclusive.
  if (has_wildcard && mnos.size() != 1) {
    return ErrorResult(type_, error);
  }
  return ErrorResult();
}

void GsmNetwork::saveLastConnectedMno(const String& mno) {
  JsonDocument mobile_config_doc;
  ErrorResult result = storage_->loadMobileConfig(mobile_config_doc);
  if (result.isError()) {
    TRACELN(result.toString());
    return;
  }
  JsonObject mobile_config = mobile_config_doc.as<JsonObject>();

  // Don't save last connected MNO if it is the same as on disk
  if (mobile_config[last_connected_mno_key_] == mno) {
    return;
  }

  mobile_config[last_connected_mno_key_] = mno.c_str();
  result = storage_->storeMobileConfig(mobile_config);
  if (result.isError()) {
    TRACELN(result.toString());
    return;
  }
}

const char* GsmNetwork::type_ = "GsmNetwork";
const char* GsmNetwork::allowed_mnos_key_ = "allowed_mnos";
const char* GsmNetwork::last_connected_mno_key_ = "last_connected_mno";

}  // namespace inamata

#endif