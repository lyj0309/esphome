#include "ld2402.h"
#include "esphome/core/application.h"
#include "esphome/core/helpers.h"

/*
HLK-LD2402 Protocol Implementation

Configure commands - little endian

All send command frames will have:
  Header = FD FC FB FA, Bytes 0 - 3, uint32_t 0xFAFBFCFD
  Length, bytes 4 - 5, uint16_t, must be at least 2 for the command byte if no addon data.
  Command bytes 6 - 7, uint16_t
  Footer = 04 03 02 01 - uint32_t 0x01020304, Always last 4 Bytes.

Receive
  Error bytes 8-9 uint16_t, 0 = success, all other positive values = error

Enable config mode:
Send:
  UART Tx: FD FC FB FA 04 00 FF 00 01 00 04 03 02 01
  Command = FF 00 - uint16_t 0x00FF
  Value = 01 00 - uint16_t 0x0001
Reply:
  UART Rx: FD FC FB FA 08 00 FF 01 00 00 02 00 20 00 04 03 02 01
  ACK = 00 00 (success), Protocol Ver = 02 00, Buffer Size = 20 00

Disable config mode:
Send:
  UART Tx: FD FC FB FA 02 00 FE 00 04 03 02 01
  Command = FE 00 - uint16_t 0x00FE
Receive:
  UART Rx: FD FC FB FA 04 00 FE 01 00 00 04 03 02 01
*/

namespace esphome {
namespace ld2402 {

static const char *const TAG = "ld2402";

// Local const's
static const uint16_t REFRESH_RATE_MS = 1000;

// Command sets
static const uint16_t CMD_DISABLE_CONF = 0x00FE;
static const uint16_t CMD_ENABLE_CONF = 0x00FF;
static const uint16_t CMD_READ_VERSION = 0x0000;
static const uint16_t CMD_READ_SERIAL_HEX = 0x0016;
static const uint16_t CMD_READ_SERIAL_STR = 0x0011;
static const uint16_t CMD_READ_PARAMETER = 0x0008;
static const uint16_t CMD_WRITE_PARAMETER = 0x0007;
static const uint16_t CMD_SET_DATA_MODE = 0x0012;
static const uint16_t CMD_AUTO_THRESHOLD = 0x0009;
static const uint16_t CMD_AUTO_THRESHOLD_PROGRESS = 0x000A;
static const uint16_t CMD_INTERFERENCE_REPORT = 0x0014;
static const uint16_t CMD_SAVE_PARAMS = 0x00FD;
static const uint16_t CMD_AUTO_GAIN = 0x00EE;
static const uint16_t CMD_AUTO_GAIN_DONE = 0x00F0;

// Parameter IDs
static const uint16_t PARAM_MAX_DISTANCE = 0x0001;
static const uint16_t PARAM_TIMEOUT = 0x0004;
static const uint16_t PARAM_POWER_INTERFERENCE = 0x0005;
static const uint16_t PARAM_MOVE_THRESH_BASE = 0x0010;  // 0x0010 to 0x001F for gates 0-15
static const uint16_t PARAM_MICRO_THRESH_BASE = 0x0030;  // 0x0030 to 0x003F for gates 0-15

// Data output modes
static const uint32_t DATA_MODE_ENGINEERING = 0x00000004;
static const uint32_t DATA_MODE_NORMAL = 0x00000064;

static const uint8_t LD2402_ERROR_NONE = 0x00;
static const uint8_t LD2402_ERROR_TIMEOUT = 0x02;
static const uint8_t LD2402_ERROR_UNKNOWN = 0x01;

// Factory defaults
static const uint32_t FACTORY_MOVE_THRESH[TOTAL_GATES] = {60000, 30000, 400, 250, 250, 250, 250, 250,
                                                          250,   250,   250, 250, 250, 250, 250, 250};
static const uint32_t FACTORY_MICRO_THRESH[TOTAL_GATES] = {40000, 20000, 200, 200, 200, 200, 200, 150,
                                                           150,   100,   100, 100, 100, 100, 100, 100};
static const uint16_t FACTORY_TIMEOUT = 5;
static const uint16_t FACTORY_MAX_DISTANCE = 100;

// COMMAND_BYTE Header & Footer
static const uint32_t CMD_FRAME_FOOTER = 0x01020304;
static const uint32_t CMD_FRAME_HEADER = 0xFAFBFCFD;
static const uint32_t DATA_FRAME_FOOTER = 0xF5F6F7F8;
static const uint32_t DATA_FRAME_HEADER = 0xF1F2F3F4;

static const uint8_t CMD_FRAME_COMMAND = 6;
static const uint8_t CMD_FRAME_DATA_LENGTH = 4;
static const uint8_t CMD_FRAME_STATUS = 7;
static const uint8_t CMD_ERROR_WORD = 8;

static const std::string OP_NORMAL_MODE_STRING = "Normal";
static const std::string OP_ENGINEERING_MODE_STRING = "Engineering";

float LD2402Component::get_setup_priority() const { return setup_priority::BUS; }

void LD2402Component::dump_config() {
  ESP_LOGCONFIG(TAG,
                "LD2402:\n"
                "  Firmware version: %s\n"
                "  Serial number: %s",
                this->firmware_ver_, this->serial_number_);
#ifdef USE_NUMBER
  ESP_LOGCONFIG(TAG, "Number:");
  LOG_NUMBER("  ", "Gate Timeout:", this->gate_timeout_number_);
  LOG_NUMBER("  ", "Max Distance:", this->max_distance_number_);
  for (uint8_t gate = 0; gate < TOTAL_GATES; gate++) {
    LOG_NUMBER("  ", "Gate Move Threshold:", this->gate_move_threshold_numbers_[gate]);
    LOG_NUMBER("  ", "Gate Micro Threshold:", this->gate_micro_threshold_numbers_[gate]);
  }
#endif
#ifdef USE_BUTTON
  LOG_BUTTON("  ", "Apply Config:", this->apply_config_button_);
  LOG_BUTTON("  ", "Revert Edits:", this->revert_config_button_);
  LOG_BUTTON("  ", "Factory Reset:", this->factory_reset_button_);
  LOG_BUTTON("  ", "Restart Module:", this->restart_module_button_);
#endif
#ifdef USE_SELECT
  ESP_LOGCONFIG(TAG, "Select:");
  LOG_SELECT("  ", "Operating Mode", this->operating_selector_);
#endif
}

void LD2402Component::setup() {
  ESP_LOGCONFIG(TAG, "Setting up LD2402...");
  
  if (this->set_config_mode(true) == LD2402_ERROR_TIMEOUT) {
    ESP_LOGE(TAG, "Failed to enable configuration mode");
    this->mark_failed();
    return;
  }
  
  this->get_firmware_version_();
  this->get_serial_number_();
  this->get_max_distance_timeout_();
  
#ifdef USE_NUMBER
  this->init_gate_config_numbers();
#endif
  
  // Read all gate thresholds
  for (uint8_t gate = 0; gate < TOTAL_GATES; gate++) {
    delay_microseconds_safe(125);
    this->get_gate_threshold_(gate);
  }
  
  memcpy(&this->new_config, &this->current_config, sizeof(this->current_config));
  
  // Set normal mode
  this->set_mode_(DATA_MODE_NORMAL);
  this->set_system_mode(this->system_mode_);
  
#ifdef USE_SELECT
  if (this->operating_selector_ != nullptr) {
    this->operating_selector_->publish_state(OP_NORMAL_MODE_STRING);
  }
#endif
  
  this->set_config_mode(false);
  ESP_LOGCONFIG(TAG, "LD2402 setup complete");
}

void LD2402Component::loop() {
  const uint32_t now = millis();
  
  // Read and process UART data
  while (this->available()) {
    uint8_t byte;
    this->read_byte(&byte);
    this->readline_(byte, this->buffer_data_, MAX_LINE_LENGTH);
  }
  
  // Periodic updates
  if (now - this->last_periodic_millis > REFRESH_RATE_MS) {
    this->last_periodic_millis = now;
    
    // Update listeners with current state
    for (auto &listener : this->listeners_) {
      listener->on_presence(this->presence_);
      listener->on_distance(this->distance_);
      listener->on_energy(this->gate_energy_, TOTAL_GATES);
    }
  }
}

void LD2402Component::readline_(int rx_data, uint8_t *buffer, int len) {
  static uint8_t pos = 0;
  
  if (rx_data >= 0) {
    if (pos < len - 1) {
      buffer[pos++] = rx_data;
      buffer[pos] = 0;
    } else {
      pos = 0;  // Buffer overflow, reset
    }
    
    // Check for complete frame
    if (pos >= 14) {  // Minimum frame size
      uint32_t *header = (uint32_t *) buffer;
      
      if (*header == CMD_FRAME_HEADER) {
        uint16_t frame_len = buffer[4] | (buffer[5] << 8);
        uint16_t total_len = frame_len + 10;  // header(4) + length(2) + data + footer(4)
        
        if (pos >= total_len) {
          uint32_t *footer = (uint32_t *) (buffer + total_len - 4);
          if (*footer == CMD_FRAME_FOOTER) {
            this->handle_ack_data_(buffer, total_len);
            pos = 0;
          }
        }
      } else if (*header == DATA_FRAME_HEADER) {
        uint16_t frame_len = buffer[4] | (buffer[5] << 8);
        uint16_t total_len = frame_len + 10;
        
        if (pos >= total_len) {
          uint32_t *footer = (uint32_t *) (buffer + total_len - 4);
          if (*footer == DATA_FRAME_FOOTER) {
            this->handle_engineering_mode_(buffer, total_len);
            pos = 0;
          }
        }
      }
    }
  }
}

void LD2402Component::handle_ack_data_(uint8_t *buffer, int len) {
  if (len < 14)
    return;
  
  uint16_t cmd = buffer[6] | (buffer[7] << 8);
  uint16_t ack = buffer[8] | (buffer[9] << 8);
  
  ESP_LOGV(TAG, "ACK received - Command: 0x%04X, Status: 0x%04X", cmd, ack);
  
  this->cmd_reply_.command = cmd;
  this->cmd_reply_.status = ack;
  this->cmd_reply_.length = len - 14;  // Exclude header, length, cmd, ack, footer
  
  // Copy data if any
  if (this->cmd_reply_.length > 0 && this->cmd_reply_.length <= sizeof(this->cmd_reply_.data)) {
    memcpy(this->cmd_reply_.data, buffer + 10, this->cmd_reply_.length);
  }
  
  this->cmd_reply_.ack = true;
  this->cmd_active_ = false;
}

void LD2402Component::handle_engineering_mode_(uint8_t *buffer, int len) {
  if (len < 43)  // Minimum size for engineering mode data
    return;
  
  // Parse engineering mode data
  // Format: Header(4) + Length(2) + Presence(1) + Distance(2) + Energy[16](32) + Footer(4)
  this->presence_ = buffer[6] != 0;
  this->distance_ = buffer[7] | (buffer[8] << 8);
  
  // Parse gate energies
  for (uint8_t gate = 0; gate < TOTAL_GATES; gate++) {
    this->gate_energy_[gate] = buffer[9 + gate * 2] | (buffer[10 + gate * 2] << 8);
  }
  
  ESP_LOGV(TAG, "Engineering mode - Presence: %d, Distance: %d cm", this->presence_, this->distance_);
}

void LD2402Component::handle_normal_mode_(const uint8_t *inbuf, int len) {
  // Handle normal mode text output
  // Format: "ON" or "OFF" for presence, "Range XXXX" for distance
  // This is a simplified implementation
}

uint8_t LD2402Component::set_config_mode(bool enable) {
  CmdFrameT cmd_frame;
  cmd_frame.header = CMD_FRAME_HEADER;
  cmd_frame.footer = CMD_FRAME_FOOTER;
  
  if (enable) {
    cmd_frame.command = CMD_ENABLE_CONF;
    cmd_frame.length = 4;
    cmd_frame.data[0] = 0x01;
    cmd_frame.data[1] = 0x00;
  } else {
    cmd_frame.command = CMD_DISABLE_CONF;
    cmd_frame.length = 2;
  }
  
  this->send_cmd_from_array(cmd_frame);
  
  // Wait for ACK
  uint32_t timeout = millis() + 1000;
  while (!this->cmd_reply_.ack && millis() < timeout) {
    App.feed_wdt();
    delay(10);
  }
  
  if (!this->cmd_reply_.ack) {
    ESP_LOGE(TAG, "Config mode %s timeout", enable ? "enable" : "disable");
    return LD2402_ERROR_TIMEOUT;
  }
  
  if (this->cmd_reply_.status != 0) {
    ESP_LOGE(TAG, "Config mode %s failed with status: 0x%04X", enable ? "enable" : "disable",
             this->cmd_reply_.status);
    return LD2402_ERROR_UNKNOWN;
  }
  
  ESP_LOGD(TAG, "Config mode %s", enable ? "enabled" : "disabled");
  return LD2402_ERROR_NONE;
}

int LD2402Component::send_cmd_from_array(CmdFrameT cmd_frame) {
  uint8_t cmd_buffer[64];
  uint8_t pos = 0;
  
  // Header
  cmd_buffer[pos++] = cmd_frame.header & 0xFF;
  cmd_buffer[pos++] = (cmd_frame.header >> 8) & 0xFF;
  cmd_buffer[pos++] = (cmd_frame.header >> 16) & 0xFF;
  cmd_buffer[pos++] = (cmd_frame.header >> 24) & 0xFF;
  
  // Length
  cmd_buffer[pos++] = cmd_frame.length & 0xFF;
  cmd_buffer[pos++] = (cmd_frame.length >> 8) & 0xFF;
  
  // Command
  cmd_buffer[pos++] = cmd_frame.command & 0xFF;
  cmd_buffer[pos++] = (cmd_frame.command >> 8) & 0xFF;
  
  // Data
  if (cmd_frame.length > 2) {
    uint8_t data_len = cmd_frame.length - 2;
    for (uint8_t i = 0; i < data_len; i++) {
      cmd_buffer[pos++] = cmd_frame.data[i];
    }
  }
  
  // Footer
  cmd_buffer[pos++] = cmd_frame.footer & 0xFF;
  cmd_buffer[pos++] = (cmd_frame.footer >> 8) & 0xFF;
  cmd_buffer[pos++] = (cmd_frame.footer >> 16) & 0xFF;
  cmd_buffer[pos++] = (cmd_frame.footer >> 24) & 0xFF;
  
  this->cmd_active_ = true;
  this->cmd_reply_.ack = false;
  
  this->write_array(cmd_buffer, pos);
  this->flush();
  
  return pos;
}

void LD2402Component::get_firmware_version_() {
  CmdFrameT cmd_frame;
  cmd_frame.header = CMD_FRAME_HEADER;
  cmd_frame.footer = CMD_FRAME_FOOTER;
  cmd_frame.command = CMD_READ_VERSION;
  cmd_frame.length = 2;
  
  this->send_cmd_from_array(cmd_frame);
  
  uint32_t timeout = millis() + 1000;
  while (!this->cmd_reply_.ack && millis() < timeout) {
    App.feed_wdt();
    delay(10);
  }
  
  if (this->cmd_reply_.ack && this->cmd_reply_.status == 0) {
    uint16_t ver_len = this->cmd_reply_.data[0] | (this->cmd_reply_.data[1] << 8);
    if (ver_len > 0 && ver_len < sizeof(this->firmware_ver_)) {
      memcpy(this->firmware_ver_, &this->cmd_reply_.data[2], ver_len);
      this->firmware_ver_[ver_len] = '\0';
      ESP_LOGD(TAG, "Firmware version: %s", this->firmware_ver_);
      
      std::string fw_str(this->firmware_ver_);
      for (auto &listener : this->listeners_) {
        listener->on_fw_version(fw_str);
      }
    }
  }
}

void LD2402Component::get_serial_number_() {
  CmdFrameT cmd_frame;
  cmd_frame.header = CMD_FRAME_HEADER;
  cmd_frame.footer = CMD_FRAME_FOOTER;
  cmd_frame.command = CMD_READ_SERIAL_STR;
  cmd_frame.length = 2;
  
  this->send_cmd_from_array(cmd_frame);
  
  uint32_t timeout = millis() + 1000;
  while (!this->cmd_reply_.ack && millis() < timeout) {
    App.feed_wdt();
    delay(10);
  }
  
  if (this->cmd_reply_.ack && this->cmd_reply_.status == 0) {
    uint16_t sn_len = this->cmd_reply_.data[0] | (this->cmd_reply_.data[1] << 8);
    if (sn_len > 0 && sn_len < sizeof(this->serial_number_)) {
      memcpy(this->serial_number_, &this->cmd_reply_.data[2], sn_len);
      this->serial_number_[sn_len] = '\0';
      ESP_LOGD(TAG, "Serial number: %s", this->serial_number_);
    }
  }
}

int LD2402Component::get_max_distance_timeout_() {
  // Read max distance parameter
  this->get_parameter_(PARAM_MAX_DISTANCE);
  
  uint32_t timeout = millis() + 1000;
  while (!this->cmd_reply_.ack && millis() < timeout) {
    App.feed_wdt();
    delay(10);
  }
  
  if (this->cmd_reply_.ack && this->cmd_reply_.status == 0) {
    this->current_config.max_distance = this->cmd_reply_.data[0] | (this->cmd_reply_.data[1] << 8) |
                                       (this->cmd_reply_.data[2] << 16) | (this->cmd_reply_.data[3] << 24);
    ESP_LOGD(TAG, "Max distance: %d", this->current_config.max_distance);
  }
  
  // Read timeout parameter
  this->cmd_reply_.ack = false;
  this->get_parameter_(PARAM_TIMEOUT);
  
  timeout = millis() + 1000;
  while (!this->cmd_reply_.ack && millis() < timeout) {
    App.feed_wdt();
    delay(10);
  }
  
  if (this->cmd_reply_.ack && this->cmd_reply_.status == 0) {
    this->current_config.timeout = this->cmd_reply_.data[0] | (this->cmd_reply_.data[1] << 8) |
                                  (this->cmd_reply_.data[2] << 16) | (this->cmd_reply_.data[3] << 24);
    ESP_LOGD(TAG, "Timeout: %d", this->current_config.timeout);
  }
  
  return LD2402_ERROR_NONE;
}

void LD2402Component::get_parameter_(uint16_t param_id) {
  CmdFrameT cmd_frame;
  cmd_frame.header = CMD_FRAME_HEADER;
  cmd_frame.footer = CMD_FRAME_FOOTER;
  cmd_frame.command = CMD_READ_PARAMETER;
  cmd_frame.length = 4;
  cmd_frame.data[0] = param_id & 0xFF;
  cmd_frame.data[1] = (param_id >> 8) & 0xFF;
  
  this->send_cmd_from_array(cmd_frame);
}

int LD2402Component::get_gate_threshold_(uint8_t gate) {
  if (gate >= TOTAL_GATES)
    return LD2402_ERROR_UNKNOWN;
  
  // Get move threshold
  this->get_parameter_(PARAM_MOVE_THRESH_BASE + gate);
  
  uint32_t timeout = millis() + 1000;
  while (!this->cmd_reply_.ack && millis() < timeout) {
    App.feed_wdt();
    delay(10);
  }
  
  if (this->cmd_reply_.ack && this->cmd_reply_.status == 0) {
    this->current_config.move_thresh[gate] = this->cmd_reply_.data[0] | (this->cmd_reply_.data[1] << 8) |
                                            (this->cmd_reply_.data[2] << 16) | (this->cmd_reply_.data[3] << 24);
  }
  
  // Get micro threshold
  this->cmd_reply_.ack = false;
  this->get_parameter_(PARAM_MICRO_THRESH_BASE + gate);
  
  timeout = millis() + 1000;
  while (!this->cmd_reply_.ack && millis() < timeout) {
    App.feed_wdt();
    delay(10);
  }
  
  if (this->cmd_reply_.ack && this->cmd_reply_.status == 0) {
    this->current_config.micro_thresh[gate] = this->cmd_reply_.data[0] | (this->cmd_reply_.data[1] << 8) |
                                             (this->cmd_reply_.data[2] << 16) | (this->cmd_reply_.data[3] << 24);
  }
  
  return LD2402_ERROR_NONE;
}

void LD2402Component::set_parameter(uint16_t param_id, uint32_t value) {
  CmdFrameT cmd_frame;
  cmd_frame.header = CMD_FRAME_HEADER;
  cmd_frame.footer = CMD_FRAME_FOOTER;
  cmd_frame.command = CMD_WRITE_PARAMETER;
  cmd_frame.length = 8;
  
  // Parameter ID
  cmd_frame.data[0] = param_id & 0xFF;
  cmd_frame.data[1] = (param_id >> 8) & 0xFF;
  
  // Parameter value
  cmd_frame.data[2] = value & 0xFF;
  cmd_frame.data[3] = (value >> 8) & 0xFF;
  cmd_frame.data[4] = (value >> 16) & 0xFF;
  cmd_frame.data[5] = (value >> 24) & 0xFF;
  
  this->send_cmd_from_array(cmd_frame);
}

void LD2402Component::set_max_distance_timeout(uint32_t max_distance, uint32_t timeout) {
  this->set_parameter(PARAM_MAX_DISTANCE, max_distance);
  
  uint32_t wait_timeout = millis() + 500;
  while (!this->cmd_reply_.ack && millis() < wait_timeout) {
    App.feed_wdt();
    delay(10);
  }
  
  this->cmd_reply_.ack = false;
  this->set_parameter(PARAM_TIMEOUT, timeout);
  
  wait_timeout = millis() + 500;
  while (!this->cmd_reply_.ack && millis() < wait_timeout) {
    App.feed_wdt();
    delay(10);
  }
}

void LD2402Component::set_gate_threshold(uint8_t gate) {
  if (gate >= TOTAL_GATES)
    return;
  
  // Set move threshold
  this->set_parameter(PARAM_MOVE_THRESH_BASE + gate, this->new_config.move_thresh[gate]);
  
  uint32_t timeout = millis() + 500;
  while (!this->cmd_reply_.ack && millis() < timeout) {
    App.feed_wdt();
    delay(10);
  }
  
  // Set micro threshold
  this->cmd_reply_.ack = false;
  this->set_parameter(PARAM_MICRO_THRESH_BASE + gate, this->new_config.micro_thresh[gate]);
  
  timeout = millis() + 500;
  while (!this->cmd_reply_.ack && millis() < timeout) {
    App.feed_wdt();
    delay(10);
  }
}

void LD2402Component::set_system_mode(uint16_t mode) {
  CmdFrameT cmd_frame;
  cmd_frame.header = CMD_FRAME_HEADER;
  cmd_frame.footer = CMD_FRAME_FOOTER;
  cmd_frame.command = CMD_SET_DATA_MODE;
  cmd_frame.length = 8;
  
  // Command value (always 0x0000)
  cmd_frame.data[0] = 0x00;
  cmd_frame.data[1] = 0x00;
  
  // Mode value
  cmd_frame.data[2] = mode & 0xFF;
  cmd_frame.data[3] = (mode >> 8) & 0xFF;
  cmd_frame.data[4] = (mode >> 16) & 0xFF;
  cmd_frame.data[5] = (mode >> 24) & 0xFF;
  
  this->send_cmd_from_array(cmd_frame);
  
  uint32_t timeout = millis() + 1000;
  while (!this->cmd_reply_.ack && millis() < timeout) {
    App.feed_wdt();
    delay(10);
  }
  
  if (this->cmd_reply_.ack && this->cmd_reply_.status == 0) {
    this->system_mode_ = mode;
    ESP_LOGD(TAG, "System mode set to: 0x%08X", mode);
  }
}

void LD2402Component::set_operating_mode(const std::string &state) {
  if (state == OP_NORMAL_MODE_STRING) {
    this->current_operating_mode = OP_NORMAL_MODE;
    this->set_mode_(DATA_MODE_NORMAL);
  } else if (state == OP_ENGINEERING_MODE_STRING) {
    this->current_operating_mode = OP_ENGINEERING_MODE;
    this->set_mode_(DATA_MODE_ENGINEERING);
  }
}

void LD2402Component::apply_config_action() {
  ESP_LOGD(TAG, "Applying configuration");
  
  if (this->set_config_mode(true) == LD2402_ERROR_TIMEOUT) {
    ESP_LOGE(TAG, "Failed to enable configuration mode");
    return;
  }
  
  this->set_max_distance_timeout(this->new_config.max_distance, this->new_config.timeout);
  
  for (uint8_t gate = 0; gate < TOTAL_GATES; gate++) {
    delay_microseconds_safe(125);
    this->set_gate_threshold(gate);
  }
  
  memcpy(&this->current_config, &this->new_config, sizeof(this->new_config));
  
#ifdef USE_NUMBER
  this->init_gate_config_numbers();
#endif
  
  this->save_parameters();
  this->set_config_mode(false);
  
  ESP_LOGD(TAG, "Configuration applied");
}

void LD2402Component::factory_reset_action() {
  ESP_LOGD(TAG, "Factory reset");
  
  if (this->set_config_mode(true) == LD2402_ERROR_TIMEOUT) {
    ESP_LOGE(TAG, "Failed to enable configuration mode");
    return;
  }
  
  this->new_config.max_distance = FACTORY_MAX_DISTANCE;
  this->new_config.timeout = FACTORY_TIMEOUT;
  
  for (uint8_t gate = 0; gate < TOTAL_GATES; gate++) {
    this->new_config.move_thresh[gate] = FACTORY_MOVE_THRESH[gate];
    this->new_config.micro_thresh[gate] = FACTORY_MICRO_THRESH[gate];
  }
  
  this->apply_config_action();
}

void LD2402Component::revert_config_action() {
  ESP_LOGD(TAG, "Reverting configuration");
  memcpy(&this->new_config, &this->current_config, sizeof(this->current_config));
#ifdef USE_NUMBER
  this->refresh_gate_config_numbers();
#endif
}

void LD2402Component::restart_module_action() {
  ESP_LOGD(TAG, "Restarting module");
  this->ld2402_restart();
}

void LD2402Component::ld2402_restart() {
  // Module restart would require specific command
  // Not documented in the provided protocol, using config mode toggle
  this->set_config_mode(false);
  delay(100);
  this->set_config_mode(true);
  delay(100);
  this->set_config_mode(false);
}

void LD2402Component::save_parameters() {
  CmdFrameT cmd_frame;
  cmd_frame.header = CMD_FRAME_HEADER;
  cmd_frame.footer = CMD_FRAME_FOOTER;
  cmd_frame.command = CMD_SAVE_PARAMS;
  cmd_frame.length = 2;
  
  this->send_cmd_from_array(cmd_frame);
  
  uint32_t timeout = millis() + 1000;
  while (!this->cmd_reply_.ack && millis() < timeout) {
    App.feed_wdt();
    delay(10);
  }
  
  if (this->cmd_reply_.ack && this->cmd_reply_.status == 0) {
    ESP_LOGD(TAG, "Parameters saved");
  }
}

void LD2402Component::power_on_auto_gain() {
  CmdFrameT cmd_frame;
  cmd_frame.header = CMD_FRAME_HEADER;
  cmd_frame.footer = CMD_FRAME_FOOTER;
  cmd_frame.command = CMD_AUTO_GAIN;
  cmd_frame.length = 2;
  
  this->send_cmd_from_array(cmd_frame);
  
  uint32_t timeout = millis() + 5000;  // Longer timeout for auto gain
  while (!this->cmd_reply_.ack && millis() < timeout) {
    App.feed_wdt();
    delay(100);
  }
  
  if (this->cmd_reply_.ack && this->cmd_reply_.status == 0) {
    ESP_LOGD(TAG, "Auto gain adjustment started");
  }
}

void LD2402Component::auto_calibrate_sensitivity(float move_factor, float micro_factor) {
  ESP_LOGD(TAG, "Starting auto calibration with factors: move=%.2f, micro=%.2f", move_factor, micro_factor);
  
  CmdFrameT cmd_frame;
  cmd_frame.header = CMD_FRAME_HEADER;
  cmd_frame.footer = CMD_FRAME_FOOTER;
  cmd_frame.command = CMD_AUTO_THRESHOLD;
  cmd_frame.length = 8;
  
  // Convert factors to parameter values (multiplied by 10)
  uint16_t move_param = static_cast<uint16_t>(move_factor * 10.0);
  uint16_t micro_param = static_cast<uint16_t>(micro_factor * 10.0);
  
  // Move threshold factor
  cmd_frame.data[0] = move_param & 0xFF;
  cmd_frame.data[1] = (move_param >> 8) & 0xFF;
  
  // Keep threshold factor (same as move)
  cmd_frame.data[2] = move_param & 0xFF;
  cmd_frame.data[3] = (move_param >> 8) & 0xFF;
  
  // Micro threshold factor
  cmd_frame.data[4] = micro_param & 0xFF;
  cmd_frame.data[5] = (micro_param >> 8) & 0xFF;
  
  this->send_cmd_from_array(cmd_frame);
  
  uint32_t timeout = millis() + 1000;
  while (!this->cmd_reply_.ack && millis() < timeout) {
    App.feed_wdt();
    delay(10);
  }
  
  if (this->cmd_reply_.ack && this->cmd_reply_.status == 0) {
    ESP_LOGD(TAG, "Auto calibration started");
  }
}

void LD2402Component::handle_cmd_error(uint8_t error) {
  switch (error) {
    case LD2402_ERROR_NONE:
      ESP_LOGD(TAG, "Command successful");
      break;
    case LD2402_ERROR_TIMEOUT:
      ESP_LOGE(TAG, "Command timeout");
      break;
    case LD2402_ERROR_UNKNOWN:
    default:
      ESP_LOGE(TAG, "Command failed with error: 0x%02X", error);
      break;
  }
}

#ifdef USE_NUMBER
void LD2402Component::init_gate_config_numbers() {
  if (this->gate_timeout_number_ != nullptr) {
    this->gate_timeout_number_->publish_state(this->current_config.timeout);
  }
  if (this->max_distance_number_ != nullptr) {
    this->max_distance_number_->publish_state(this->current_config.max_distance);
  }
  this->refresh_gate_config_numbers();
}

void LD2402Component::refresh_gate_config_numbers() {
  for (uint8_t gate = 0; gate < TOTAL_GATES; gate++) {
    if (this->gate_move_threshold_numbers_[gate] != nullptr) {
      this->gate_move_threshold_numbers_[gate]->publish_state(this->new_config.move_thresh[gate]);
    }
    if (this->gate_micro_threshold_numbers_[gate] != nullptr) {
      this->gate_micro_threshold_numbers_[gate]->publish_state(this->new_config.micro_thresh[gate]);
    }
  }
}
#endif

}  // namespace ld2402
}  // namespace esphome
