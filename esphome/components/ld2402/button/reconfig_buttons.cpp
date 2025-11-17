#include "reconfig_buttons.h"

namespace esphome {
namespace ld2402 {

void LD2402ApplyConfigButton::press_action() { this->parent_->apply_config_action(); }

void LD2402RevertConfigButton::press_action() { this->parent_->revert_config_action(); }

void LD2402RestartModuleButton::press_action() { this->parent_->restart_module_action(); }

void LD2402FactoryResetButton::press_action() { this->parent_->factory_reset_action(); }

}  // namespace ld2402
}  // namespace esphome
