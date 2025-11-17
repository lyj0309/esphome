import esphome.codegen as cg
from esphome.components import select
import esphome.config_validation as cv
from esphome.const import ENTITY_CATEGORY_CONFIG

from .. import CONF_LD2402_ID, LD2402Component, ld2402_ns

CONF_OPERATING_MODE = "operating_mode"
CONF_SELECTS = [
    "Normal",
    "Engineering",
]

LD2402Select = ld2402_ns.class_("LD2402Select", cg.Component)

CONFIG_SCHEMA = {
    cv.GenerateID(CONF_LD2402_ID): cv.use_id(LD2402Component),
    cv.Required(CONF_OPERATING_MODE): select.select_schema(
        LD2402Select,
        entity_category=ENTITY_CATEGORY_CONFIG,
    ),
}


async def to_code(config):
    LD2402_component = await cg.get_variable(config[CONF_LD2402_ID])
    if operating_mode_config := config.get(CONF_OPERATING_MODE):
        sel = await select.new_select(
            operating_mode_config,
            options=[CONF_SELECTS],
        )
        await cg.register_parented(sel, config[CONF_LD2402_ID])
        cg.add(LD2402_component.set_operating_mode_select(sel))
