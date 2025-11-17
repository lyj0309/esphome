#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/automation.h"
#include "esphome/core/helpers.h"
#ifdef USE_TEXT_SENSOR
#include "esphome/components/text_sensor/text_sensor.h"
#endif
#ifdef USE_SELECT
#include "esphome/components/select/select.h"
#endif
#ifdef USE_NUMBER
#include "esphome/components/number/number.h"
#endif
#ifdef USE_BUTTON
#include "esphome/components/button/button.h"
#endif

namespace esphome {
namespace ld2402 {

static const uint8_t MAX_LINE_LENGTH = 64;  // Max characters for serial buffer
static const uint8_t TOTAL_GATES = 16;

enum OpMode : uint8_t {
  OP_NORMAL_MODE = 1,
  OP_ENGINEERING_MODE = 2,
};

class LD2402Listener {
 public:
  virtual void on_presence(bool presence){};
  virtual void on_distance(uint16_t distance){};
  virtual void on_energy(uint16_t *sensor_energy, size_t size){};
  virtual void on_fw_version(std::string &fw){};
};

class LD2402Component : public Component, public uart::UARTDevice {
 public:
  struct CmdFrameT {
    uint32_t header{0};
    uint32_t footer{0};
    uint16_t length{0};
    uint16_t command{0};
    uint16_t data_length{0};
    uint8_t data[32];
  };

  struct RegConfigT {
    uint32_t move_thresh[TOTAL_GATES];
    uint32_t micro_thresh[TOTAL_GATES];
    uint16_t max_distance{0};
    uint16_t timeout{0};
  };

  void setup() override;
  void dump_config() override;
  void loop() override;
#ifdef USE_SELECT
  void set_operating_mode_select(select::Select *selector) { this->operating_selector_ = selector; };
#endif
#ifdef USE_NUMBER
  void set_gate_timeout_number(number::Number *number) { this->gate_timeout_number_ = number; };
  void set_max_distance_number(number::Number *number) { this->max_distance_number_ = number; };
  void set_gate_move_threshold_numbers(int gate, number::Number *n) { this->gate_move_threshold_numbers_[gate] = n; };
  void set_gate_micro_threshold_numbers(int gate, number::Number *n) { this->gate_micro_threshold_numbers_[gate] = n; };
  void init_gate_config_numbers();
  void refresh_gate_config_numbers();
#endif
#ifdef USE_BUTTON
  void set_apply_config_button(button::Button *button) { this->apply_config_button_ = button; };
  void set_revert_config_button(button::Button *button) { this->revert_config_button_ = button; };
  void set_restart_module_button(button::Button *button) { this->restart_module_button_ = button; };
  void set_factory_reset_button(button::Button *button) { this->factory_reset_button_ = button; };
#endif
  void register_listener(LD2402Listener *listener) { this->listeners_.push_back(listener); }

  void send_module_restart();
  void restart_module_action();
  void apply_config_action();
  void factory_reset_action();
  void revert_config_action();
  float get_setup_priority() const override;
  int send_cmd_from_array(CmdFrameT cmd_frame);
  void handle_cmd_error(uint8_t error);
  void set_operating_mode(const std::string &state);
  void auto_calibrate_sensitivity(float move_factor, float micro_factor);
  uint8_t set_config_mode(bool enable);
  void set_max_distance_timeout(uint32_t max_distance, uint32_t timeout);
  void set_gate_threshold(uint8_t gate);
  void set_parameter(uint16_t param_id, uint32_t value);
  void get_parameter(uint16_t param_id);
  void set_system_mode(uint16_t mode);
  void ld2402_restart();
  void save_parameters();
  void power_on_auto_gain();

  float gate_move_sensitivity_factor{3.0};
  float gate_micro_sensitivity_factor{3.0};
  int32_t last_periodic_millis{0};
  int32_t report_periodic_millis{0};
  int32_t monitor_periodic_millis{0};
  int32_t last_normal_periodic_millis{0};
  uint16_t gate_energy_[TOTAL_GATES];
  uint8_t current_operating_mode{OP_NORMAL_MODE};
  bool output_energy_state{false};
  RegConfigT current_config;
  RegConfigT new_config;
#ifdef USE_SELECT
  select::Select *operating_selector_{nullptr};
#endif
#ifdef USE_BUTTON
  button::Button *apply_config_button_{nullptr};
  button::Button *revert_config_button_{nullptr};
  button::Button *restart_module_button_{nullptr};
  button::Button *factory_reset_button_{nullptr};
#endif

 protected:
  struct CmdReplyT {
    uint32_t data[8];
    uint16_t error;
    uint8_t command;
    uint8_t status;
    uint8_t length;
    volatile bool ack;
  };

  void get_firmware_version_();
  void get_serial_number_();
  int get_gate_threshold_(uint8_t gate);
  void get_parameter_(uint16_t param_id);
  int get_max_distance_timeout_();
  uint16_t get_mode_() { return this->system_mode_; };
  void set_mode_(uint16_t mode) { this->system_mode_ = mode; };
  bool get_presence_() { return this->presence_; };
  void set_presence_(bool presence) { this->presence_ = presence; };
  uint16_t get_distance_() { return this->distance_; };
  void set_distance_(uint16_t distance) { this->distance_ = distance; };
  void handle_normal_mode_(const uint8_t *inbuf, int len);
  void handle_engineering_mode_(uint8_t *buffer, int len);
  void handle_ack_data_(uint8_t *buffer, int len);
  void readline_(int rx_data, uint8_t *buffer, int len);

#ifdef USE_NUMBER
  number::Number *gate_timeout_number_{nullptr};
  number::Number *max_distance_number_{nullptr};
  std::vector<number::Number *> gate_micro_threshold_numbers_ = std::vector<number::Number *>(16);
  std::vector<number::Number *> gate_move_threshold_numbers_ = std::vector<number::Number *>(16);
#endif

  uint16_t distance_{0};
  uint16_t system_mode_;
  uint8_t buffer_pos_{0};  // where to resume processing/populating buffer
  uint8_t buffer_data_[MAX_LINE_LENGTH];
  char firmware_ver_[16]{"v0.0.0"};
  char serial_number_[32]{"Unknown"};
  bool cmd_active_{false};
  bool presence_{false};
  CmdReplyT cmd_reply_;
  std::vector<LD2402Listener *> listeners_{};
};

}  // namespace ld2402
}  // namespace esphome
