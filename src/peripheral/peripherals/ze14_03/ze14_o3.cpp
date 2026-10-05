#ifdef DEVICE_TYPE_MULTI_AIR_SENSOR

#include "ze14_o3.h"

#include "managers/uart_manager.h"
#include "peripheral/peripheral_factory.h"
#include "utils/error_store.h"

namespace inamata {
namespace peripheral {
namespace peripherals {
namespace ze14_03 {

namespace {
constexpr uint8_t kReadCommand[9] = {0xFF, 0x01, 0x86, 0x00, 0x00,
                                     0x00, 0x00, 0x00, 0x79};
constexpr uint8_t kResponseSize = 9;
constexpr uint8_t kResponseHeader = 0xFF;
constexpr uint8_t kResponseCommand = 0x86;
}  // namespace

ZE14_O3::ZE14_O3(const JsonObjectConst& parameters) {
  data_point_type_ = utils::UUID(parameters[data_point_type_key_]);
  if (!data_point_type_.isValid()) {
    setInvalid(ErrorStore::genMissingProperty(data_point_type_key_,
                                              ErrorStore::KeyType::kUUID));
    return;
  }

  int tx_pin = toPin(parameters[tx_key_]);
  if (tx_pin < 0) {
    setInvalid(
        ErrorStore::genMissingProperty(tx_key_, ErrorStore::KeyType::kUint32t));
    return;
  }

  int rx_pin = toPin(parameters[rx_key_]);
  if (rx_pin < 0) {
    setInvalid(
        ErrorStore::genMissingProperty(rx_key_, ErrorStore::KeyType::kUint32t));
    return;
  }

  serial_ = UartManager::getUartInterface();
  if (serial_ == nullptr) {
    setInvalid("No UART available");
    return;
  }

  serial_->begin(9600, SERIAL_8N1, rx_pin, tx_pin);
}

const String& ZE14_O3::getType() const { return type(); }

const String& ZE14_O3::type() {
  static const String name{"ZE14_O3"};
  return name;
}

capabilities::StartMeasurement::Result ZE14_O3::startMeasurement(
    const JsonVariantConst& parameters) {
  // Multiple tasks may share this peripheral; let a request already in
  // flight finish instead of resetting the shared UART buffer mid-read
  if (state_ == State::kWaiting) {
    return {.wait = measurement_wait_};
  }

  // Serve other consumers the cached reading until it goes stale
  if (state_ == State::kReady &&
      std::chrono::steady_clock::now() - ready_time_ <
          min_measurement_interval_) {
    return {.wait = {}};
  }

  issueRequest();
  return {.wait = measurement_wait_};
}

capabilities::StartMeasurement::Result ZE14_O3::handleMeasurement() {
  if (state_ == State::kIdle) {
    return {.wait = {}, .error = ErrorResult(type(), "Not started")};
  }

  if (state_ == State::kReady) {
    return {.wait = {}};
  }

  if (std::chrono::steady_clock::now() - request_time_ > measurement_timeout_) {
    // Go back to idle so the next startMeasurement() can retry instead of
    // being stuck thinking a request is forever in flight
    state_ = State::kIdle;
    return {.wait = {}, .error = ErrorResult(type(), "Timeout")};
  }

  if (!readResponse()) {
    return {.wait = measurement_wait_};
  }

  if (response_[0] != kResponseHeader || response_[1] != kResponseCommand) {
    // Retry within the same cycle; keep request_time_ so the overall
    // timeout still bounds repeated garbage/misaligned responses
    response_i_ = 0;
    memset(response_, 0, sizeof(response_));
    writeCommand();
    return {.wait = measurement_wait_};
  }

  const uint16_t ozone_ppb =
      (uint16_t(response_[2]) << 8) | uint16_t(response_[3]);
  last_ozone_ppb_ = static_cast<float>(ozone_ppb);
  state_ = State::kReady;
  ready_time_ = std::chrono::steady_clock::now();
  return {.wait = {}};
}

capabilities::GetValues::Result ZE14_O3::getValues() {
  if (state_ != State::kReady || std::isnan(last_ozone_ppb_)) {
    return {.values = {}, .error = ErrorResult(type(), get_values_error_)};
  }

  return {.values = {utils::ValueUnit(last_ozone_ppb_, data_point_type_)},
          .error = ErrorResult()};
}

std::shared_ptr<Peripheral> ZE14_O3::factory(
    const ServiceGetters& services, const JsonObjectConst& parameters) {
  return std::make_shared<ZE14_O3>(parameters);
}

void ZE14_O3::issueRequest() {
  state_ = State::kWaiting;
  request_time_ = std::chrono::steady_clock::now();
  last_ozone_ppb_ = NAN;
  response_i_ = 0;
  memset(response_, 0, sizeof(response_));
  writeCommand();
}

void ZE14_O3::writeCommand() {
  while (serial_->available()) {
    serial_->read();
  }
  serial_->write(kReadCommand, sizeof(kReadCommand));
}

bool ZE14_O3::readResponse() {
  while (serial_->available() > 0 && response_i_ < kResponseSize) {
    const int byte_read = serial_->read();
    if (byte_read < 0) {
      break;
    }
    response_[response_i_++] = static_cast<uint8_t>(byte_read);
  }

  return response_i_ >= kResponseSize;
}

bool ZE14_O3::registered_ = PeripheralFactory::registerFactory(type(), factory);

bool ZE14_O3::capability_get_values_ =
    capabilities::GetValues::registerType(type());

bool ZE14_O3::capability_start_measurement_ =
    capabilities::StartMeasurement::registerType(type());

}  // namespace ze14_03
}  // namespace peripherals
}  // namespace peripheral
}  // namespace inamata

#endif