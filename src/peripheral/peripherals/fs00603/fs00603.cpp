#ifdef DEVICE_TYPE_MULTI_AIR_SENSOR

#include "fs00603.h"

#include <uart.h>

#include "managers/logging.h"
#include "managers/uart_manager.h"
#include "peripheral/peripheral_factory.h"
#include "utils/chrono.h"
#include "utils/error_store.h"

namespace inamata {
namespace peripheral {
namespace peripherals {
namespace fs00603 {

FS00603::FS00603(const JsonObjectConst& parameters) {
  co2_data_point_type_ = utils::UUID(parameters[co2_data_point_type_key_]);
  if (!co2_data_point_type_.isValid()) {
    setInvalid(ErrorStore::genMissingProperty(co2_data_point_type_key_,
                                              ErrorStore::KeyType::kUUID));
    return;
  }

  voc_data_point_type_ = utils::UUID(parameters[voc_data_point_type_key_]);
  if (!voc_data_point_type_.isValid()) {
    setInvalid(ErrorStore::genMissingProperty(voc_data_point_type_key_,
                                              ErrorStore::KeyType::kUUID));
    return;
  }

  formaldehyde_data_point_type_ =
      utils::UUID(parameters[formaldehyde_data_point_type_key_]);
  if (!formaldehyde_data_point_type_.isValid()) {
    setInvalid(ErrorStore::genMissingProperty(formaldehyde_data_point_type_key_,
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
  serial_->onReceiveError(
      [this](hardwareSerial_error_t error) { onUartError(error); });
}

void FS00603::onUartError(hardwareSerial_error_t error) {
  // Fires from the UART event task; only used to confirm/rule out HW-level
  // overflow as the cause of framing errors seen after running for a while
  const char* name = "unknown";
  switch (error) {
    case UART_BREAK_ERROR:
      name = "break";
      uart_break_detected_ = true;
      break;
    case UART_BUFFER_FULL_ERROR:
      name = "buffer_full";
      break;
    case UART_FIFO_OVF_ERROR:
      name = "fifo_overflow";
      break;
    case UART_FRAME_ERROR:
      name = "frame";
      break;
    case UART_PARITY_ERROR:
      name = "parity";
      break;
    default:
      break;
  }
  TRACEF("FS00603 UART error: %s\r\n", name);
}

const String& FS00603::getType() const { return type(); }

const String& FS00603::type() {
  static const String name{"FS00603"};
  return name;
}

bool FS00603::hasCachedMeasurement() const {
  return measurement_ready_ &&
         !utils::hasTimedOut(measurement_ready_time_, kCacheDuration);
}

capabilities::StartMeasurement::Result FS00603::startMeasurement(
    const JsonVariantConst& parameters) {
  if (hasCachedMeasurement()) {
    // TRACELN("FS00603 startMeasurement: cached");
    return {.wait = {}};
  }

  if (!started_) {
    resetState();
    started_ = true;
    // TRACEF("FS00603 startMeasurement: first start, wait=%lldms\r\n",
    //        (long long)kInitialWait.count());
    return {.wait = kInitialWait};
  }

  // Only handleMeasurement() drives readFrame(), so a completed frame always
  // goes through its measurement_ready_/resetState() bookkeeping - otherwise
  // a stray success here would leave in_data_i_ stuck at frame_size_ forever
  measurement_ready_ = false;
  //   TRACELN(
  //       "FS00603 startMeasurement: already started, deferring to "
  //       "handleMeasurement");
  return {.wait = {}};
}

capabilities::StartMeasurement::Result FS00603::handleMeasurement() {
  if (!started_) {
    return {.wait = {}, .error = ErrorResult(type(), "Not started")};
  }

  // Drain whatever is queued even while a cached reading is still valid.
  // The OEM sketch never lets bytes sit unread; leaving them queued for up
  // to kCacheDuration was likely letting the RX buffer/backlog build up and
  // desync framing over time.
  capabilities::StartMeasurement::Result result = readFrame();
  if (!result.error.isError() &&
      result.wait == std::chrono::nanoseconds::zero()) {
    measurement_ready_ = true;
    measurement_ready_time_ = std::chrono::steady_clock::now();
    resetState();
  }

  if (hasCachedMeasurement()) {
    return {.wait = {}};
  }

  const auto elapsed = std::chrono::steady_clock::now() - measurement_start_;
  if (elapsed > measurement_timeout_) {
    resetState();
    return capabilities::StartMeasurement::Result{
        .error = ErrorResult(type(), "Timeout")};
  }

  return result;
}

capabilities::GetValues::Result FS00603::getValues() {
  if (!measurement_ready_) {
    return capabilities::GetValues::Result{
        .error = ErrorResult(type(), get_values_error_)};
  }

  capabilities::GetValues::Result result;
  result.values.push_back(
      utils::ValueUnit(last_co2_ppm_, co2_data_point_type_));
  result.values.push_back(
      utils::ValueUnit(last_voc_ppm_, voc_data_point_type_));
  result.values.push_back(
      utils::ValueUnit(last_formaldehyde_ugm3_, formaldehyde_data_point_type_));

  return result;
}

capabilities::StartMeasurement::Result FS00603::readFrame() {
  if (uart_break_detected_) {
    uart_break_detected_ = false;
    in_data_i_ = 0;
  }

  bool received_valid_frame = false;

  while (serial_->available()) {
    const uint8_t incoming_byte = serial_->read();

    // Wait for the start byte before beginning a packet.
    if (in_data_i_ == 0) {
      if (incoming_byte == frame_header_) {
        in_data_[in_data_i_++] = incoming_byte;
      }
      continue;
    }

    // Once started, retain every byte until the complete 9-byte packet is
    // available, matching the OEM reference sketch's framing behavior.
    in_data_[in_data_i_++] = incoming_byte;
    if (in_data_i_ >= frame_size_) {
      const uint8_t calculated_checksum = calculateChecksum(in_data_);
      if (calculated_checksum != in_data_[frame_size_ - 1]) {
        // TRACEF(
        //     "FS00603 readFrame: checksum mismatch, got=%02X expected=%02X, "
        //     "in_data_=%02X %02X %02X %02X %02X %02X %02X %02X %02X\r\n",
        //     in_data_[frame_size_ - 1], calculated_checksum, in_data_[0],
        //     in_data_[1], in_data_[2], in_data_[3], in_data_[4], in_data_[5],
        //     in_data_[6], in_data_[7], in_data_[8]);

        // A lost byte makes the next frame's 0xFF header appear inside this
        // invalid candidate. Keep that suffix so the next received bytes can
        // complete the frame rather than discarding its beginning.
        uint8_t next_frame_start = 0;
        for (uint8_t i = 1; i < frame_size_; i++) {
          if (in_data_[i] == frame_header_) {
            next_frame_start = i;
          }
        }
        if (next_frame_start > 0) {
          in_data_i_ = frame_size_ - next_frame_start;
          memmove(in_data_, in_data_ + next_frame_start, in_data_i_);
        } else {
          in_data_i_ = 0;
        }
        continue;
      }

      last_co2_ppm_ = (static_cast<uint16_t>(in_data_[2]) << 8) | in_data_[3];
      last_voc_ppm_ = (static_cast<uint16_t>(in_data_[4]) << 8) | in_data_[5];
      last_formaldehyde_ugm3_ =
          (static_cast<uint16_t>(in_data_[6]) << 8) | in_data_[7];
      Serial.printf("CO2: %f, VOC: %f, Form: %f\r\n", last_co2_ppm_,
                    last_voc_ppm_, last_formaldehyde_ugm3_);
      received_valid_frame = true;
      in_data_i_ = 0;
    }
  }

  if (received_valid_frame) {
    return capabilities::StartMeasurement::Result();
  }

  return capabilities::StartMeasurement::Result{.wait = no_data_wait_};
}

void FS00603::resetState() {
  // Clear frame data
  memset(in_data_, 0, sizeof(in_data_));
  in_data_i_ = 0;
  // Start time of reading a new frame
  measurement_start_ = std::chrono::steady_clock::now();
  uart_flush_input(UART_NUM_2);
}

uint8_t FS00603::calculateChecksum(const uint8_t* data) {
  uint8_t sum = 0;
  for (uint8_t i = 1; i <= 7; i++) {
    sum += data[i];
  }
  return (~sum) + 1;
}

std::shared_ptr<Peripheral> FS00603::factory(
    const ServiceGetters& services, const JsonObjectConst& parameters) {
  return std::make_shared<FS00603>(parameters);
}

bool FS00603::registered_ = PeripheralFactory::registerFactory(type(), factory);

bool FS00603::capability_get_values_ =
    capabilities::GetValues::registerType(type());

bool FS00603::capability_start_measurement_ =
    capabilities::StartMeasurement::registerType(type());

const __FlashStringHelper* FS00603::co2_data_point_type_key_ =
    FPSTR("co2_data_point_type");
const __FlashStringHelper* FS00603::voc_data_point_type_key_ =
    FPSTR("voc_data_point_type");
const __FlashStringHelper* FS00603::formaldehyde_data_point_type_key_ =
    FPSTR("formaldehyde_data_point_type");

}  // namespace fs00603
}  // namespace peripherals
}  // namespace peripheral
}  // namespace inamata

#endif
