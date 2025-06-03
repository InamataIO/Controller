#pragma once

#include <NimBLEDevice.h>

#include <memory>

#include "logging.h"

namespace inamata {

class BleServer {
  enum class State {
    kDISABLED,
    kENABLED,
  };

 public:
  using UserDataHandler = std::function<bool(const JsonObjectConst& data)>;

  BleServer() = default;
  ~BleServer() = default;

  void enable();
  void disable();
  bool isActive();

  NimBLEService* createService(const NimBLEUUID& uuid);

  std::vector<UserDataHandler> user_data_handlers_;

 private:
  bool setup();

  State state_{State::kDISABLED};

  // Memory managed by NimBLE library
  NimBLEServer* server_{nullptr};
};

}  // namespace inamata