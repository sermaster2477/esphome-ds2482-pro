import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import CONF_ADDRESS, CONF_CHANNEL, CONF_ID
from . import ds2482_ns, DS2482Component, CONF_DS2482_ID

# Добавляем константу для овердрайва
CONF_OVERDRIVE = "overdrive"

DS2482Sensor = ds2482_ns.class_('DS2482Sensor', sensor.Sensor, cg.PollingComponent)

CONFIG_SCHEMA = sensor.sensor_schema(
    DS2482Sensor,
    unit_of_measurement="°C",
    accuracy_decimals=1,
    device_class="temperature",
    state_class="measurement",
).extend({
    cv.GenerateID(CONF_DS2482_ID): cv.use_id(DS2482Component),
    cv.Required(CONF_ADDRESS): cv.hex_uint64_t,
    cv.Required(CONF_CHANNEL): cv.int_range(min=0, max=7),
    # ТЕПЕРЬ ОШИБКИ НЕ БУДЕТ:
    cv.Optional(CONF_OVERDRIVE, default=False): cv.boolean,
}).extend(cv.polling_component_schema("60s"))

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await sensor.register_sensor(var, config)
    
    parent = await cg.get_variable(config[CONF_DS2482_ID])
    cg.add(var.set_parent(parent))
    cg.add(var.set_address(config[CONF_ADDRESS]))
    cg.add(var.set_channel(config[CONF_CHANNEL]))
    # Передаем значение в C++ (убедитесь, что в .h классе есть set_overdrive)
    if CONF_OVERDRIVE in config:
        cg.add(var.set_overdrive(config[CONF_OVERDRIVE]))