#include "managers/ble_server.h"

#include "managers/storage.h"

namespace inamata {

void BleServer::enable() {
  if (state_ == State::kENABLED) {
    return;
  }
  TRACELN(F("Enabling BleServer"));
  setup();
  state_ = State::kENABLED;
}

void BleServer::disable() {
  if (state_ == State::kDISABLED) {
    return;
  }
  TRACELN(F("Disabling BleServer"));
  NimBLEDevice::deinit(true);
  state_ = State::kDISABLED;
}

bool BleServer::isActive() { return state_ == State::kENABLED; }

NimBLEService* BleServer::createService(const NimBLEUUID& uuid) {
  if (state_ != State::kENABLED || !server_) {
    enable();
  }
  return server_->createService(uuid);
}

bool BleServer::setup() {
  NimBLEDevice::init(Storage::device_type_name_);
  // NimBLEDevice::setPower(ESP_PWR_LVL_P9);  // +9db (default is 3db)
  NimBLEDevice::setSecurityAuth(BLE_SM_PAIR_AUTHREQ_SC);
  server_ = NimBLEDevice::createServer();
  return true;
}

}  // namespace inamata
