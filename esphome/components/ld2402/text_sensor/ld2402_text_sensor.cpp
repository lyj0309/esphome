#include "ld2402_text_sensor.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ld2402 {

static const char *const TAG = "ld2402.text_sensor";

void LD2402TextSensor::dump_config() {
  ESP_LOGCONFIG(TAG, "LD2402 Text Sensor:");
  LOG_TEXT_SENSOR("  ", "Firmware Version", this->fw_version_text_sensor_);
  LOG_TEXT_SENSOR("  ", "Serial Number", this->serial_number_text_sensor_);
}

}  // namespace ld2402
}  // namespace esphome
