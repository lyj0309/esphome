#pragma once

#include "../ld2402.h"
#include "esphome/components/text_sensor/text_sensor.h"

namespace esphome {
namespace ld2402 {

class LD2402TextSensor : public LD2402Listener, public Component, text_sensor::TextSensor {
 public:
  void dump_config() override;
  void set_fw_version_text_sensor(text_sensor::TextSensor *tsensor) { this->fw_version_text_sensor_ = tsensor; };
  void set_serial_number_text_sensor(text_sensor::TextSensor *tsensor) { this->serial_number_text_sensor_ = tsensor; };
  void on_fw_version(std::string &fw) override {
    if (this->fw_version_text_sensor_ != nullptr) {
      this->fw_version_text_sensor_->publish_state(fw);
    }
  }

 protected:
  text_sensor::TextSensor *fw_version_text_sensor_{nullptr};
  text_sensor::TextSensor *serial_number_text_sensor_{nullptr};
};

}  // namespace ld2402
}  // namespace esphome
