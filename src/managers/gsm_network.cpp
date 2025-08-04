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

GsmNetwork::GsmNetwork()
    :
#ifdef DUMP_AT_COMMANDS
      debugger_(SerialAT, SerialMon),
      modem_(debugger_),
#else
      modem_(SerialAT),
#endif
      client_(modem_) {
  pinMode(peripheral::fixed::gsm_enable_pin, OUTPUT);
  pinMode(peripheral::fixed::gsm_reset_pin, OUTPUT);
  SerialAT.begin(115200, SERIAL_8N1, peripheral::fixed::gsm_rx_pin,
                 peripheral::fixed::gsm_tx_pin);
}

void GsmNetwork::enable() {
  digitalWrite(peripheral::fixed::gsm_enable_pin, HIGH);
  delay(200);
  digitalWrite(peripheral::fixed::gsm_reset_pin, LOW);
  delay(200);
  digitalWrite(peripheral::fixed::gsm_reset_pin, HIGH);
  delay(2000);
  modem_.init();

  is_enabled_ = true;
}

void GsmNetwork::disable() {
  digitalWrite(peripheral::fixed::gsm_enable_pin, LOW);

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
  const auto now = std::chrono::steady_clock::now();
  if (utils::chrono_abs(now - last_network_check_) > check_period_) {
    last_network_check_ = now;

    network_connected_ = modem_.isNetworkConnected();
    if (network_connected_) {
      gprs_connected_ = modem_.isGprsConnected();
      signal_quality_ = modem_.getSignalQuality();
    } else {
      gprs_connected_ = false;
      signal_quality_ = 0;
    }
    TRACEF("Network: %d GPRS: %d CSQ: %d\r\n", network_connected_,
           gprs_connected_, signal_quality_);

    if (network_connected_ && !gprs_connected_) {
      if (modem_.isGprsConnected()) {
        TRACELN("GRPS already connected");
        gprs_connected_ = true;
      } else {
        TRACELN("GRPS connecting");
        gprs_connected_ = modem_.gprsConnect(GSM_APN);
      }
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

}  // namespace inamata

#endif