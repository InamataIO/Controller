#include "uart_manager.h"

#include "managers/logging.h"

namespace inamata {

HardwareSerial* UartManager::getUartInterface() {
  if (!uart_1_used_) {
    uart_1_used_ = true;
    return &Serial1;
  } else if (!uart_2_used_) {
    uart_2_used_ = true;
    return &Serial2;
  }
  TRACELN("No UART available");
  return nullptr;
}

bool UartManager::uart_1_used_ = false;
bool UartManager::uart_2_used_ = false;

}  // namespace inamata