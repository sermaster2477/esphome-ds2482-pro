import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c
from esphome.const import CONF_ID

# Это то, что искал лог ошибки
CONF_DS2482_ID = "ds2482_id"

DEPENDENCIES = ['i2c']
AUTO_LOAD = ['sensor']

ds2482_ns = cg.esphome_ns.namespace('ds2482')
DS2482Component = ds2482_ns.class_('DS2482Component', cg.Component, i2c.I2CDevice)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(DS2482Component),
}).extend(cv.COMPONENT_SCHEMA).extend(i2c.i2c_device_schema(0x18))

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)