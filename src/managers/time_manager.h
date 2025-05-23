#include <Arduino.h>
#include <RTClib.h>

namespace inamata {

class TimeManager {
 public:
  static void initRTC();
  static DateTime systemTime();
  static void setSystemTime(int year, int month, int day, int hour, int minute,
                            int second);

  static DateTime getPastDate(int days);
  static String getFormattedTime();
  static String getCurrentDate();
  static void showSystemDateTime();

  static bool isValidYear(String year);
  static bool isValidMonth(String month);
  static bool isValidDay(String day, int inYear, int inMonth);
  static bool isValidHour(String hour);
  static bool isValidMinute(String minute);
};
}  // namespace inamata