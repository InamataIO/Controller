#include "ads1x15_input.h"

#include "managers/services.h"

namespace inamata {
namespace peripheral {
namespace peripherals {
namespace ads1x15 {

ADS1X15Input::ADS1X15Input(const JsonObjectConst& parameters) {
  utils::UUID ads1x15_adapter_uuid(parameters[ads1x15_adapter_key_]);
  if (!ads1x15_adapter_uuid.isValid()) {
    setInvalid(ads1x15_adapter_key_error_);
    return;
  }

  std::shared_ptr<Peripheral> peripheral =
      Services::getPeripheralController().getPeripheral(ads1x15_adapter_uuid);
  if (!peripheral) {
    setInvalid(peripheralNotFoundError(ads1x15_adapter_uuid));
    return;
  }

  // Since the UUID is specified externally, check the type
  if (peripheral->getType() == ADS1X15Adapter::type() &&
      peripheral->isValid()) {
    ads1x15_adapter_ = std::static_pointer_cast<ADS1X15Adapter>(peripheral);
  } else {
    setInvalid(notAValidError(ads1x15_adapter_uuid, peripheral->getType()));
    return;
  }

  String error = parseParameters(parameters);
  if (!error.isEmpty()) {
    setInvalid(error);
  }

  const char* channel = parameters[channel_key_].as<const char*>();
  if (strcmp(channel, "ch0") == 0) {
    channel_ = ADS1X15_REG_CONFIG_MUX_SINGLE_0;
  } else if (strcmp(channel, "ch1") == 0) {
    channel_ = ADS1X15_REG_CONFIG_MUX_SINGLE_1;
  } else if (strcmp(channel, "ch2") == 0) {
    channel_ = ADS1X15_REG_CONFIG_MUX_SINGLE_2;
  } else if (strcmp(channel, "ch3") == 0) {
    channel_ = ADS1X15_REG_CONFIG_MUX_SINGLE_3;
  } else if (strcmp(channel, "dif01") == 0) {
    channel_ = ADS1X15_REG_CONFIG_MUX_DIFF_0_1;
  } else if (strcmp(channel, "dif02") == 0) {
    channel_ = ADS1X15_REG_CONFIG_MUX_DIFF_0_3;
  } else if (strcmp(channel, "dif13") == 0) {
    channel_ = ADS1X15_REG_CONFIG_MUX_DIFF_1_3;
  } else if (strcmp(channel, "dif23") == 0) {
    channel_ = ADS1X15_REG_CONFIG_MUX_DIFF_2_3;
  } else {
    setInvalid(channel_key_error_);
  }

  // Set the gain
  JsonVariantConst gain = parameters[gain_key_];
  if (!gain.isNull()) {
    const char* gain_str = gain.as<const char*>();
    if (strcmp(gain_str, "x2/3") == 0) {
      gain_ = GAIN_TWOTHIRDS;
    } else if (strcmp(gain_str, "x1") == 0) {
      gain_ = GAIN_ONE;
    } else if (strcmp(gain_str, "x2") == 0) {
      gain_ = GAIN_TWO;
    } else if (strcmp(gain_str, "x4") == 0) {
      gain_ = GAIN_FOUR;
    } else if (strcmp(gain_str, "x8") == 0) {
      gain_ = GAIN_EIGHT;
    } else if (strcmp(gain_str, "x16") == 0) {
      gain_ = GAIN_SIXTEEN;
    } else {
      setInvalid(gain_key_error_);
    }
  }
}

const String& ADS1X15Input::getType() const { return type(); }

const String& ADS1X15Input::type() {
  static const String name{"ADS1X15Input"};
  return name;
}

capabilities::StartMeasurement::Result ADS1X15Input::startMeasurement(
    const JsonVariantConst& parameters) {
  return ads1x15_adapter_->startADCReading(
      {.channel = channel_, .gain = gain_, .caller = static_cast<void*>(this)});
}

capabilities::StartMeasurement::Result ADS1X15Input::handleMeasurement() {
  const auto wait = ads1x15_adapter_->isFinished(static_cast<void*>(this));
  if (wait < std::chrono::nanoseconds::zero()) {
    return ads1x15_adapter_->startADCReading(
        {.channel = channel_,
         .gain = gain_,
         .caller = static_cast<void*>(this)});
  }
  return {.wait = wait};
}

capabilities::GetValues::Result ADS1X15Input::getValues() {
  std::vector<utils::ValueUnit> values;

  ADS1X15Adapter::Result result =
      ads1x15_adapter_->completeReading(static_cast<void*>(this));
  if (result.error.isError()) {
    return {.values = {}, .error = result.error};
  }

  if (voltage_data_point_type_.isValid()) {
    values.push_back(
        {utils::ValueUnit(result.voltage, voltage_data_point_type_)});
  }
  if (unit_data_point_type_.isValid()) {
    float unit_value = min_unit_ + v_to_unit_slope_ * (result.voltage - min_v_);
    if (limit_unit_) {
      // Constrain mapped value between min and max unit
      if (min_unit_ < max_unit_) {
        unit_value = std::max(min_unit_, std::min(unit_value, max_unit_));
      } else {
        unit_value = std::max(max_unit_, std::min(unit_value, min_unit_));
      }
    }
    values.push_back({utils::ValueUnit(unit_value, unit_data_point_type_)});
  }

  return {.values = values, .error = {}};
}

std::shared_ptr<Peripheral> ADS1X15Input::factory(
    const ServiceGetters& services, const JsonObjectConst& parameters) {
  return std::make_shared<ADS1X15Input>(parameters);
}

bool ADS1X15Input::registered_ =
    PeripheralFactory::registerFactory(type(), factory);

bool ADS1X15Input::capability_get_values_ =
    capabilities::GetValues::registerType(type());

const __FlashStringHelper* ADS1X15Input::channel_key_ = FPSTR("channel");
const __FlashStringHelper* ADS1X15Input::channel_key_error_ =
    FPSTR("Missing property: channel (str)");
const __FlashStringHelper* ADS1X15Input::gain_key_ = FPSTR("gain");
const __FlashStringHelper* ADS1X15Input::gain_key_error_ =
    FPSTR("Invalid property: gain (str)");
const __FlashStringHelper* ADS1X15Input::ads1x15_adapter_key_ =
    FPSTR("ads1x15_adapter");
const __FlashStringHelper* ADS1X15Input::ads1x15_adapter_key_error_ =
    FPSTR("Missing property: ads1x15_adapter (uuid)");

}  // namespace ads1x15
}  // namespace peripherals
}  // namespace peripheral
}  // namespace inamata
