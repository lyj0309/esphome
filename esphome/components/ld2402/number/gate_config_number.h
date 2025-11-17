#pragma once

#include "esphome/components/number/number.h"
#include "../ld2402.h"

namespace esphome {
namespace ld2402 {

class LD2402TimeoutNumber : public number::Number, public Parented<LD2402Component> {
 public:
  LD2402TimeoutNumber() = default;

 protected:
  void control(float timeout) override;
};

class LD2402MaxDistanceNumber : public number::Number, public Parented<LD2402Component> {
 public:
  LD2402MaxDistanceNumber() = default;

 protected:
  void control(float max_distance) override;
};

class LD2402MicroThresholdNumbers : public number::Number, public Parented<LD2402Component> {
 public:
  LD2402MicroThresholdNumbers() = default;
  LD2402MicroThresholdNumbers(uint8_t gate);

 protected:
  uint8_t gate_;
  void control(float micro_threshold) override;
};

class LD2402MoveThresholdNumbers : public number::Number, public Parented<LD2402Component> {
 public:
  LD2402MoveThresholdNumbers() = default;
  LD2402MoveThresholdNumbers(uint8_t gate);

 protected:
  uint8_t gate_;
  void control(float move_threshold) override;
};

}  // namespace ld2402
}  // namespace esphome
