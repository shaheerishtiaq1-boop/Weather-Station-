idf_component_register(SRCS "mqttDrv.c"
                       INCLUDE_DIRS "."
                       REQUIRES driver 
                       mqttDrv 
                       esp_timer 
                       esp_driver_gpio)