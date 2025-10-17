#include "gate_config_number.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

static const char *const TAG = "ld2402.number";

namespace esphome {
namespace ld2402 {

void LD2402TimeoutNumber::control(float timeout) {
  this->publish_state(timeout);
  this->parent_->new_config.timeout = timeout;
}

void LD2402MaxDistanceNumber::control(float max_distance) {
  this->publish_state(max_distance);
  this->parent_->new_config.max_distance = (uint16_t) max_distance;
}

LD2402MoveThresholdNumbers::LD2402MoveThresholdNumbers(uint8_t gate) : gate_(gate) {}

void LD2402MoveThresholdNumbers::control(float move_threshold) {
  this->publish_state(move_threshold);
  this->parent_->new_config.move_thresh[this->gate_] = move_threshold;
}

LD2402MicroThresholdNumbers::LD2402MicroThresholdNumbers(uint8_t gate) : gate_(gate) {}

void LD2402MicroThresholdNumbers::control(float micro_threshold) {
  this->publish_state(micro_threshold);
  this->parent_->new_config.micro_thresh[this->gate_] = micro_threshold;
}

}  // namespace ld2402
}  // namespace esphome
