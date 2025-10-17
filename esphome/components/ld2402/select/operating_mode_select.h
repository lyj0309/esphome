#pragma once

#include "../ld2402.h"
#include "esphome/components/select/select.h"

namespace esphome {
namespace ld2402 {

class LD2402Select : public Component, public select::Select, public Parented<LD2402Component> {
 public:
  LD2402Select() = default;

 protected:
  void control(const std::string &value) override;
};

}  // namespace ld2402
}  // namespace esphome
