#pragma once

#include "../ld2402.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

namespace esphome {
namespace ld2402 {

class LD2402BinarySensor : public LD2402Listener, public Component, binary_sensor::BinarySensor {
 public:
  void dump_config() override;
  void set_presence_sensor(binary_sensor::BinarySensor *bsensor) { this->presence_bsensor_ = bsensor; };
  void on_presence(bool presence) override {
    if (this->presence_bsensor_ != nullptr) {
      if (this->presence_bsensor_->state != presence)
        this->presence_bsensor_->publish_state(presence);
    }
  }

 protected:
  binary_sensor::BinarySensor *presence_bsensor_{nullptr};
};

}  // namespace ld2402
}  // namespace esphome
