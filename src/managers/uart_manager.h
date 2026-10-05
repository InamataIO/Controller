#pragma once

#include <Arduino.h>

namespace inamata {

class UartManager {
 public:
  /**
   * Return available UART interface
   */
  static HardwareSerial* getUartInterface();

 private:
  // Serial/Serial0 reserved for terminal. Serial1 and Serial2 open for use
  static bool uart_1_used_;
  static bool uart_2_used_;
};

}  // namespace inamata