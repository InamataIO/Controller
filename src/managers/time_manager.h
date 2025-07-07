#ifdef RTC_MANAGER

#include <ArduinoJson.h>
#include <RTClib.h>

namespace inamata {

class TimeManager {
 public:
  static void initRTC();
  static DateTime systemTime();
  static void setSystemTime(const DateTime& date_time);
  static void setSystemTime(int year, int month, int day, int hour, int minute,
                            int second);
  static bool lostPower();

  static DateTime getPastDate(int days);
  static String getFormattedTime(const DateTime& date_time);
  static String getFormattedTime();
  static String getCurrentDate(const DateTime& date_time);
  static String getCurrentDate();

  static bool handleImprovUserData(const JsonObjectConst& data);

  static bool isValidYear(String year);
  static bool isValidMonth(String month);
  static bool isValidDay(String day, int in_year, int in_month);
  static bool isValidHour(String hour);
  static bool isValidMinute(String minute);

  static bool isLostPowerOnBoot();

 private:
  // On boot check if the RTC lost power
  static bool lost_power_on_boot_;
};
}  // namespace inamata

#endif