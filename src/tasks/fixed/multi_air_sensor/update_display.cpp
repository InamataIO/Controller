#ifdef DEVICE_TYPE_MULTI_AIR_SENSOR
#include "update_display.h"

#include <chrono>
#include <cmath>
#include <memory>

#include "aging.h"
#include "managers/logging.h"
#include "peripheral/fixed.h"
#include "utils/chrono.h"

namespace inamata {
namespace tasks {
namespace fixed {

using namespace std::placeholders;

UpdateDisplay::UpdateDisplay(const ServiceGetters& services,
                             Scheduler& scheduler,
                             const JsonObjectConst& behavior_config)
    : BaseTask(scheduler, Input(nullptr, true)),
      gsm_network_(services.getGsmNetwork()) {
  // device_config is the JSON stored in LittleFS
  JsonDocument fixed_config_doc;
  Storage::loadFixedConfig(fixed_config_doc);

  JsonObjectConst task_config = findConfig(fixed_config_doc["tasks"]);
  if (task_config.isNull()) {
    setInvalid("Config not found");
    return;
  }

  auto& peripheral_controller = Services::getPeripheralController();
  JsonArrayConst sensors = task_config["sensors"];
  String peripheral_error("Peripheral not found: ");
  for (JsonObjectConst sensor : sensors) {
    TRACEF("Using: %s\r\n", sensor["name"].as<const char*>());
    auto peripheral = peripheral_controller.getPeripheral(sensor["uuid"]);
    if (peripheral == nullptr) {
      setInvalid(peripheral_error + sensor["uuid"].as<const char*>());
      return;
    }
    sensors_.push_back({.peripheral = peripheral,
                        .data_point_type = sensor["dpt"],
                        .name = sensor["name"].as<const char*>(),
                        .unit = sensor["unit"].as<const char*>(),
                        .decimals = sensor["decimals"].as<uint8_t>()});
  }

  display_ = std::dynamic_pointer_cast<P_U8G2>(
      peripheral_controller.getPeripheral(task_config["display"]));
  if (display_ == nullptr) {
    setInvalid(peripheral_error + task_config["display"].as<const char*>());
    return;
  }

  setIterations(TASK_FOREVER);
  enable();
}

const String& UpdateDisplay::getType() const { return type(); }

const String& UpdateDisplay::type() {
  static const String name{"UpdateDisplay"};
  return name;
}

JsonObjectConst UpdateDisplay::findConfig(JsonArrayConst tasks_config) {
  const String& type_name = type();
  for (JsonObjectConst task : tasks_config) {
    if (task["type"] == type_name) {
      task.isNull();
      return task;
    }
  }
  return JsonObjectConst();
}

bool UpdateDisplay::TaskCallback() {
  std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
  std::chrono::nanoseconds min_wait = std::chrono::nanoseconds::max();
  bool sensor_updated = false;

  // Start new measurements for all sensors that support capability
  if (utils::hasTimedOut(last_measurement_start_, measurement_interval_, now)) {
    // TRACEF("Start %lld, %lld, %lld\r\n", last_measurement_start_,
    //        measurement_interval_, now);
    last_measurement_start_ = now;

    for (Sensor& sensor : sensors_) {
      sensor.wait_until = now;
      auto measure_sensor =
          std::dynamic_pointer_cast<StartMeasurement>(sensor.peripheral);
      if (measure_sensor != nullptr) {
        StartMeasurement::Result result =
            measure_sensor->startMeasurement(JsonVariantConst());
        if (result.error.isError()) {
          TRACEF("Error startMeasurement: %s\r\n",
                 result.error.toString().c_str());
          sensor.state = Sensor::State::kIdle;
          continue;
        }
        sensor.state = Sensor::State::kHandleMeasurement;
        sensor.wait_until = now + result.wait;
        // TRACEF("Start w: %" PRId64 " %" PRId64 "\r\n", result.wait.count(),
        //        sensor.wait_until.time_since_epoch().count());
        if (result.wait < min_wait) {
          min_wait = result.wait;
        }
      } else {
        sensor.state = Sensor::State::kGetValues;
      }
    }
  }

  for (Sensor& sensor : sensors_) {
    if (sensor.state == Sensor::State::kHandleMeasurement &&
        sensor.wait_until <= now) {
      auto measure_sensor =
          std::dynamic_pointer_cast<StartMeasurement>(sensor.peripheral);
      if (measure_sensor != nullptr) {
        StartMeasurement::Result result = measure_sensor->handleMeasurement();
        if (result.error.isError()) {
          TRACEF("Error handleMeasurement: %s\r\n",
                 result.error.toString().c_str());
          sensor.state = Sensor::State::kIdle;
          continue;
        }
        if (result.wait == std::chrono::nanoseconds::zero()) {
          sensor.state = Sensor::State::kGetValues;
        }
        sensor.wait_until = now + result.wait;
        // TRACEF("Handle w: %" PRId64 " %" PRId64 "\r\n", result.wait.count(),
        //        sensor.wait_until.time_since_epoch().count());
        if (result.wait < min_wait) {
          min_wait = result.wait;
        }
      }
    }

    if (sensor.state == Sensor::State::kGetValues) {
      auto get_values_sensor =
          std::dynamic_pointer_cast<GetValues>(sensor.peripheral);
      if (get_values_sensor != nullptr) {
        GetValues::Result result = get_values_sensor->getValues();
        if (result.error.isError()) {
          TRACEF("Error getValues: %s\r\n", result.error.toString().c_str());
          sensor.state = Sensor::State::kIdle;
          continue;
        }
        // If DPT is set, use that value, else take the first one
        for (const utils::ValueUnit& value : result.values) {
          if (!sensor.data_point_type.isValid() ||
              value.data_point_type == sensor.data_point_type) {
            sensor.value = value.value;
            sensor_updated = true;
            break;
          }
        }
        // TRACELN("Updated");
        sensor.state = Sensor::State::kUpdated;
      }
    }
  }

  if (sensor_updated) {
    updateDisplay();
  }

  if (min_wait == std::chrono::nanoseconds::max()) {
    // TRACELN("Wait");
    Task::delay(std::chrono::milliseconds(default_interval_).count());
  } else {
    int64_t wait =
        std::chrono::duration_cast<std::chrono::milliseconds>(min_wait).count();
    // TRACEF("Wait %" PRId64 "\r\n", wait);
    Task::delay(wait);
  }

  return true;
}

void UpdateDisplay::updateDisplay() {
  ::U8G2* driver = display_->getDriver();
  if (driver == nullptr) {
    TRACELN("Driver nullptr");
    return;
  }

  driver->clearBuffer();
  y_offset_ = 0;
  x_offset_ = 0;

  // Move x_offset to 0, y_offset to line height (ascent + descent)
  const uint16_t aging_hours_remaining = Aging::getMaxHoursRemaining();
  if (aging_hours_remaining > 0 &&
      utils::hasTimedOut(last_aging_toggle_, aging_toggle_interval_)) {
    show_aging_message_ = !show_aging_message_;
    last_aging_toggle_ = std::chrono::steady_clock::now();
  }
  if (aging_hours_remaining > 0 && show_aging_message_) {
    drawAgingMessage(aging_hours_remaining);
  } else {
    drawStatusBar();
  }

  const size_t sensor_count = sensors_.size();
  if (sensor_count <= 2) {
    driver->setFont(u8g2_font_10x20_tr);
  } else {
    driver->setFont(u8g2_font_6x10_tr);
  }
  const u8g2_uint_t ascent = driver->getAscent();
  const u8g2_uint_t line_height = ascent - driver->getDescent();
  if (sensor_count <= 2) {
    driver->setFont(u8g2_font_6x10_tr);
  }

  // Add loop around all sensors
  y_offset_ += ascent;
  for (const Sensor& sensor : sensors_) {
    y_offset_++;

    // Draw sensor name
    String buffer;
    if (sensor.name.length()) {
      buffer = sensor.name + ' ';
      driver->drawStr(x_offset_, y_offset_, buffer.c_str());
      x_offset_ = x_offset_ + driver->getStrWidth(buffer.c_str());
    }

    // Draw value
    buffer = String(sensor.value, (unsigned int)sensor.decimals);
    if (sensor_count <= 2) {
      driver->setFont(u8g2_font_10x20_tr);
    }
    driver->drawStr(x_offset_, y_offset_, buffer.c_str());
    x_offset_ = x_offset_ + driver->getStrWidth(buffer.c_str());
    if (sensor_count <= 2) {
      driver->setFont(u8g2_font_6x10_tr);
    }

    // Draw unit
    x_offset_ += 2;
    if (sensor.unit.length()) {
      driver->drawStr(x_offset_, y_offset_, sensor.unit.c_str());
    }

    y_offset_ += line_height;
    x_offset_ = 0;
  }
  y_offset_ -= driver->getDescent();

  // Flip the ticker on a fixed cadence rather than once per redraw, since
  // sensors with independent query timings redraw at irregular rates
  if (utils::hasTimedOut(last_ticker_toggle_, ticker_interval_)) {
    update_ticker_ = !update_ticker_;
    last_ticker_toggle_ = std::chrono::steady_clock::now();
  }
  if (update_ticker_) {
    driver->drawLine(127, 0, 127, 63);
  }

  driver->sendBuffer();
}

void UpdateDisplay::drawStatusBar() {
  String buffer;
  const CheckConnectivity::Mode mode = CheckConnectivity::getMode();
  if (mode == CheckConnectivity::Mode::ConnectWiFi) {
    buffer += ("WiFi ");
    const wl_status_t status = WiFi.status();
    if (status == WL_IDLE_STATUS) {
      buffer += "idle";
    } else if (status == WL_CONNECTED) {
      buffer +=
          String(std::fminf(std::fmaxf(2 * (WiFi.RSSI() + 100), 0.0), 100.0),
                 0) +
          "% ";
      buffer += WiFi.SSID();
    } else {
      buffer += "connecting";
    }
  } else if (mode == CheckConnectivity::Mode::ConnectGsm) {
    buffer += ("Mob: ");

    buffer += gsm_network_->signal_quality_ * 3 + "% ";
    buffer += gsm_network_->getNetworkSystemModeName() + ' ';
    buffer += gsm_network_->current_mno_;
  } else {
    buffer = "Setup device";
  }

  ::U8G2* driver = display_->getDriver();
  driver->setFont(u8g2_font_6x10_tr);
  y_offset_ += driver->getAscent();
  driver->drawStr(x_offset_, y_offset_, buffer.c_str());
  y_offset_ -= driver->getDescent();
  x_offset_ = 0;
}

void UpdateDisplay::drawAgingMessage(uint16_t hours_remaining) {
  String buffer = "Aging: " + String(hours_remaining) + "H";

  ::U8G2* driver = display_->getDriver();
  driver->setFont(u8g2_font_6x10_tr);
  y_offset_ += driver->getAscent();
  driver->drawStr(x_offset_, y_offset_, buffer.c_str());
  y_offset_ -= driver->getDescent();
  x_offset_ = 0;
}

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata

#endif