import esphome.codegen as cg
from esphome.components import sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    CONF_DISTANCE,
    DEVICE_CLASS_DISTANCE,
    UNIT_CENTIMETER,
)

from .. import CONF_LD2402_ID, LD2402Component, ld2402_ns

LD2402Sensor = ld2402_ns.class_("LD2402Sensor", sensor.Sensor, cg.Component)

CONF_GATE_ENERGY = "gate_energy"

CONFIG_SCHEMA = cv.All(
    cv.COMPONENT_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(LD2402Sensor),
            cv.GenerateID(CONF_LD2402_ID): cv.use_id(LD2402Component),
            cv.Optional(CONF_DISTANCE): sensor.sensor_schema(
                device_class=DEVICE_CLASS_DISTANCE, unit_of_measurement=UNIT_CENTIMETER
            ),
        }
    ),
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    if CONF_DISTANCE in config:
        sens = await sensor.new_sensor(config[CONF_DISTANCE])
        cg.add(var.set_distance_sensor(sens))
    if CONF_GATE_ENERGY in config:
        sens = await sensor.new_sensor(config[CONF_GATE_ENERGY])
        cg.add(var.set_energy_sensor(sens))
    ld2402 = await cg.get_variable(config[CONF_LD2402_ID])
    cg.add(ld2402.register_listener(var))
