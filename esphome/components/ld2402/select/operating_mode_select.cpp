#include "operating_mode_select.h"

namespace esphome {
namespace ld2402 {

void LD2402Select::control(const std::string &value) {
  this->publish_state(value);
  this->parent_->set_operating_mode(value);
}

}  // namespace ld2402
}  // namespace esphome
