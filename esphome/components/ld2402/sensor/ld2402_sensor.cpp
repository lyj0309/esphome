#include "ld2402_sensor.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ld2402 {

static const char *const TAG = "ld2402.sensor";

void LD2402Sensor::dump_config() {
  ESP_LOGCONFIG(TAG, "LD2402 Sensor:");
  LOG_SENSOR("  ", "Distance", this->distance_sensor_);
}

}  // namespace ld2402
}  // namespace esphome
