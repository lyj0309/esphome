import esphome.codegen as cg
from esphome.components import button
import esphome.config_validation as cv
from esphome.const import (
    CONF_FACTORY_RESET,
    DEVICE_CLASS_RESTART,
    ENTITY_CATEGORY_CONFIG,
    ENTITY_CATEGORY_DIAGNOSTIC,
    ICON_DATABASE,
    ICON_RESTART,
    ICON_RESTART_ALERT,
)

from .. import CONF_LD2402_ID, LD2402Component, ld2402_ns

LD2402ApplyConfigButton = ld2402_ns.class_("LD2402ApplyConfigButton", button.Button)
LD2402RevertConfigButton = ld2402_ns.class_("LD2402RevertConfigButton", button.Button)
LD2402RestartModuleButton = ld2402_ns.class_("LD2402RestartModuleButton", button.Button)
LD2402FactoryResetButton = ld2402_ns.class_("LD2402FactoryResetButton", button.Button)

CONF_APPLY_CONFIG = "apply_config"
CONF_REVERT_CONFIG = "revert_config"
CONF_RESTART_MODULE = "restart_module"


CONFIG_SCHEMA = {
    cv.GenerateID(CONF_LD2402_ID): cv.use_id(LD2402Component),
    cv.Required(CONF_APPLY_CONFIG): button.button_schema(
        LD2402ApplyConfigButton,
        device_class=DEVICE_CLASS_RESTART,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon=ICON_RESTART_ALERT,
    ),
    cv.Optional(CONF_REVERT_CONFIG): button.button_schema(
        LD2402RevertConfigButton,
        device_class=DEVICE_CLASS_RESTART,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon=ICON_RESTART,
    ),
    cv.Optional(CONF_RESTART_MODULE): button.button_schema(
        LD2402RestartModuleButton,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        icon=ICON_DATABASE,
    ),
    cv.Optional(CONF_FACTORY_RESET): button.button_schema(
        LD2402FactoryResetButton,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon=ICON_DATABASE,
    ),
}


async def to_code(config):
    ld2402_component = await cg.get_variable(config[CONF_LD2402_ID])
    if apply_config := config.get(CONF_APPLY_CONFIG):
        b = await button.new_button(apply_config)
        await cg.register_parented(b, config[CONF_LD2402_ID])
        cg.add(ld2402_component.set_apply_config_button(b))
    if revert_config := config.get(CONF_REVERT_CONFIG):
        b = await button.new_button(revert_config)
        await cg.register_parented(b, config[CONF_LD2402_ID])
        cg.add(ld2402_component.set_revert_config_button(b))
    if restart_config := config.get(CONF_RESTART_MODULE):
        b = await button.new_button(restart_config)
        await cg.register_parented(b, config[CONF_LD2402_ID])
        cg.add(ld2402_component.set_restart_module_button(b))
    if factory_reset := config.get(CONF_FACTORY_RESET):
        b = await button.new_button(factory_reset)
        await cg.register_parented(b, config[CONF_LD2402_ID])
        cg.add(ld2402_component.set_factory_reset_button(b))
