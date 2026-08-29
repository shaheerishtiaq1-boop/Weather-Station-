idf_component_register(SRCS "mqttapp.c"
                       INCLUDE_DIRS "."
                       REQUIRES drivers DHT BH1750 wifi_manager OLED json)