#ifndef DHT22_H
#define DHT22_H

#ifdef __cplusplus
extern "C" {
#endif

#include "OneWiredrv.h"

typedef struct
{
    one_wire_t oneWire;

    float temperature;

    float humidity;

} dht22_t;

esp_err_t dht22_init(dht22_t *dht,
                     gpio_num_t gpio);

esp_err_t dht22_read(dht22_t *dht);

float dht22_get_temperature(dht22_t *dht);

float dht22_get_humidity(dht22_t *dht);

/**
 * @brief Convenience one-call starter: initializes the sensor on the given
 *        GPIO and spawns a background FreeRTOS task that reads it every
 *        ~2.5s and logs the results. Fire-and-forget for simple use in
 *        app_main().
 *
 * @param gpio  GPIO the DHT22 data line is connected to.
 * @return ESP_OK if init and task creation succeeded.
 */
esp_err_t dht22_start(gpio_num_t gpio);

#ifdef __cplusplus
}
#endif

#endif