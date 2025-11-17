import esphome.codegen as cg
from esphome.components import number
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    CONF_MOVE_THRESHOLD,
    DEVICE_CLASS_DISTANCE,
    ENTITY_CATEGORY_CONFIG,
    ICON_MOTION_SENSOR,
    ICON_TIMELAPSE,
    UNIT_SECOND,
)

from .. import CONF_LD2402_ID, LD2402Component, ld2402_ns

LD2402TimeoutNumber = ld2402_ns.class_("LD2402TimeoutNumber", number.Number)
LD2402MaxDistanceNumber = ld2402_ns.class_("LD2402MaxDistanceNumber", number.Number)
LD2402MoveThresholdNumbers = ld2402_ns.class_(
    "LD2402MoveThresholdNumbers", number.Number
)
LD2402MicroThresholdNumbers = ld2402_ns.class_(
    "LD2402MicroThresholdNumbers", number.Number
)

CONF_MAX_DISTANCE = "max_distance"
CONF_MICRO_THRESHOLD = "micro_threshold"
CONF_PRESENCE_TIMEOUT = "presence_timeout"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_LD2402_ID): cv.use_id(LD2402Component),
        cv.Optional(CONF_PRESENCE_TIMEOUT): number.number_schema(
            LD2402TimeoutNumber,
            unit_of_measurement=UNIT_SECOND,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_TIMELAPSE,
        ),
        cv.Optional(CONF_MAX_DISTANCE): number.number_schema(
            LD2402MaxDistanceNumber,
            device_class=DEVICE_CLASS_DISTANCE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_MOTION_SENSOR,
        ),
    }
)
CONFIG_SCHEMA = CONFIG_SCHEMA.extend(
    {
        cv.Optional(f"gate_{x}"): (
            {
                cv.Required(CONF_MOVE_THRESHOLD): number.number_schema(
                    LD2402MoveThresholdNumbers,
                    entity_category=ENTITY_CATEGORY_CONFIG,
                    icon=ICON_MOTION_SENSOR,
                ),
                cv.Required(CONF_MICRO_THRESHOLD): number.number_schema(
                    LD2402MicroThresholdNumbers,
                    entity_category=ENTITY_CATEGORY_CONFIG,
                    icon=ICON_MOTION_SENSOR,
                ),
            }
        )
        for x in range(16)
    }
)


async def to_code(config):
    LD2402_component = await cg.get_variable(config[CONF_LD2402_ID])
    if gate_timeout_config := config.get(CONF_PRESENCE_TIMEOUT):
        n = await number.new_number(
            gate_timeout_config, min_value=0, max_value=65535, step=5
        )
        await cg.register_parented(n, config[CONF_LD2402_ID])
        cg.add(LD2402_component.set_gate_timeout_number(n))
    if max_distance_config := config.get(CONF_MAX_DISTANCE):
        n = await number.new_number(
            max_distance_config, min_value=7, max_value=100, step=1
        )
        await cg.register_parented(n, config[CONF_LD2402_ID])
        cg.add(LD2402_component.set_max_distance_number(n))
    
    for x in range(16):
        if gate_conf := config.get(f"gate_{x}"):
            move_config = gate_conf[CONF_MOVE_THRESHOLD]
            n = cg.new_Pvariable(move_config[CONF_ID], x)
            await number.register_number(
                n, move_config, min_value=0, max_value=65535, step=25
            )
            await cg.register_parented(n, config[CONF_LD2402_ID])
            cg.add(LD2402_component.set_gate_move_threshold_numbers(x, n))

            micro_config = gate_conf[CONF_MICRO_THRESHOLD]
            n = cg.new_Pvariable(micro_config[CONF_ID], x)
            await number.register_number(
                n, micro_config, min_value=0, max_value=65535, step=25
            )
            await cg.register_parented(n, config[CONF_LD2402_ID])
            cg.add(LD2402_component.set_gate_micro_threshold_numbers(x, n))
