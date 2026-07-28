idf_component_register(
    SRCS "Peripherals/I2C/i2cdrv.c"
         "Peripherals/OneWire/dht22dri.c"
         "Protocols/mqttDrv/mqttDrv.c"
    INCLUDE_DIRS "Peripherals/I2C"
                 "Peripherals/OneWire"
                 "Protocols/mqttDrv"
    PRIV_REQUIRES driver
)