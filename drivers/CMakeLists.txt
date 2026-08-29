idf_component_register(
    SRCS "Peripherals/I2C/i2cdrv.c"
         "Peripherals/OneWire/OneWiredrv.c"
         "Peripherals/GPIOs/gpio.c"
         "Peripherals/GPIOs/mosfet.c"
         "Peripherals/GPIOs/relay.c"
         "Protocols/mqttDrv/mqttDrv.c"
    INCLUDE_DIRS "Peripherals/I2C"
                 "Peripherals/GPIOs"
                 "Peripherals/OneWire"
                 "Protocols/mqttDrv"
    REQUIRES driver esp_timer esp_driver_gpio mqtt
)