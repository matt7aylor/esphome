import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, uart

from esphome.const import (
    CONF_ID,
    CONF_CO2,
    DEVICE_CLASS_CARBON_DIOXIDE,
    STATE_CLASS_MEASUREMENT,
    UNIT_PARTS_PER_MILLION,
    ICON_MOLECULE_CO2,
)

DEPENDENCIES = ["uart"]

tes902_ns = cg.esphome_ns.namespace("tes902")
TES902 = tes902_ns.class_("TES902Component", cg.PollingComponent, uart.UARTDevice)


CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(TES902),
            cv.Optional(CONF_CO2): sensor.sensor_schema(
                unit_of_measurement=UNIT_PARTS_PER_MILLION,
                icon=ICON_MOLECULE_CO2,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_CARBON_DIOXIDE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
        }
    )
    .extend(cv.polling_component_schema("10s"))
    .extend(uart.UART_DEVICE_SCHEMA)
)

FINAL_VALIDATE_SCHEMA = uart.final_validate_device_schema(
    "tes902", baud_rate=9600, require_rx=True, require_tx=True
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    if CONF_CO2 in config:
        sens = await sensor.new_sensor(config[CONF_CO2])
        cg.add(var.set_co2_sensor(sens))
