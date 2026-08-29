idf_component_register(SRCS "wifi.c"
                    INCLUDE_DIRS "."
                    REQUIRES 
                    drivers 
                    esp_event 
                    esp_wifi 
                    esp_netif 
                    nvs_flash)
