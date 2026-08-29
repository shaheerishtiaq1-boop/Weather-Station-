#include <stdio.h>
// #include <freertos/FreeRTOS.h>
// #include <freertos/tasks.h>


// #include "bh1750.c"
// #include "DHT22.c"
// #include "gpio.c"
// #include "ssd1306.c"
#include "DHT22.h"
#include "BH1750.h"
#include "i2cdrv.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"

#include "wifi.h"
#include "mqttapp.h"
#include "ssd1306.h"

#define DHT22_GPIO       GPIO_NUM_2
 
#define I2C_PORT         I2C_NUM_0
#define I2C_SDA_GPIO     GPIO_NUM_8
#define I2C_SCL_GPIO     GPIO_NUM_9
 
#define OLED_ADDRESS     0x3C

void app_main(void)
{
   
    //  i2c_drv_t i2c;

    // ESP_ERROR_CHECK(
    //     i2c_drv_init(&i2c,
    //                  I2C_NUM_0,
    //                  GPIO_NUM_8,      // SDA
    //                  GPIO_NUM_9,      // SCL
    //                  100000));

    // dht22_start(GPIO_NUM_2);

    // bh1750_start(&i2c);

        // ESP_ERROR_CHECK(wifi_init());

    // wifi_wait_connected();

    // while (1)
    // {
    //     vTaskDelay(pdMS_TO_TICKS(1000));
    // }


    //  mqttapp_start();

    //  i2c_drv_t i2c;
    // i2c_drv_init(&i2c, I2C_NUM_0, GPIO_NUM_8, GPIO_NUM_9, 400000);
 
    // ssd1306_t oled;
    // ssd1306_init(&oled, &i2c, 0x3C);
 
    // ssd1306_clear(&oled);
    // ssd1306_set_cursor(&oled, 0, 0);
    // ssd1306_print(&oled, "Hello");
    // ssd1306_set_cursor(&oled, 0, 8);
    //  ssd1306_print(&oled, "This is the main disp");
    //  ssd1306_set_cursor(&oled, 0, 16);
    //  ssd1306_print(&oled, "lay for my project Im");
    //    ssd1306_set_cursor(&oled, 0, 24);
    //  ssd1306_print(&oled, "going to show the");
    //     ssd1306_set_cursor(&oled, 0, 32);
    //  ssd1306_print(&oled, "temp, hunidity and lux values here");
    // ssd1306_display(&oled);
 
    // while (1) {
    //     vTaskDelay(pdMS_TO_TICKS(1000));
    // }

    //  i2c_drv_t i2c;
    // i2c_drv_init(&i2c, I2C_PORT, I2C_SDA_GPIO, I2C_SCL_GPIO, 400000);
 
    // dht22_t dht;
    // dht22_init(&dht, DHT22_GPIO);
 
    // bh1750_t bh1750;
    // bh1750_init(&bh1750, &i2c);
 
    // ssd1306_t oled;
    // ssd1306_init(&oled, &i2c, OLED_ADDRESS);
 
    // char line[24];
 
    // while (1) {
    //     dht22_read(&dht);
    //     bh1750_read(&bh1750);
 
    //     ssd1306_clear(&oled);
 
    //     ssd1306_set_cursor(&oled, 0, 0);
    //     snprintf(line, sizeof(line), "Temp: %.1f C", dht22_get_temperature(&dht));
    //     ssd1306_print(&oled, line);
 
    //     ssd1306_set_cursor(&oled, 0, 8);
    //     snprintf(line, sizeof(line), "Hum:  %.1f %%", dht22_get_humidity(&dht));
    //     ssd1306_print(&oled, line);
 
    //     ssd1306_set_cursor(&oled, 0, 16);
    //     snprintf(line, sizeof(line), "Lux:  %.1f", bh1750_get_lux(&bh1750));
    //     ssd1306_print(&oled, line);
 
    //     ssd1306_display(&oled);
 
    //     vTaskDelay(pdMS_TO_TICKS(2500));
    // }
       
    ESP_ERROR_CHECK(wifi_init());
    mqttapp_start();
     
}
