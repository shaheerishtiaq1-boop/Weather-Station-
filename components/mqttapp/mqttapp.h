/**
 * @file mqttapp.h
 * @brief Application layer: reads DHT22 + BH1750 and publishes telemetry
 *        to ThingsBoard over MQTT.
 *
 * Owns the sensor and MQTT driver instances directly (does not use
 * dht22_start()/bh1750_start()'s standalone logging tasks) so that both
 * readings are available together when building the telemetry payload.
 */

#ifndef MQTTAPP_H
#define MQTTAPP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

/**
 * @brief Convenience one-call starter.
 *
 * Waits for WiFi to connect, initializes the I2C bus, DHT22, BH1750, and
 * MQTT client, then runs a background task that periodically reads both
 * sensors and publishes a JSON telemetry payload to ThingsBoard.
 *
 * @return ESP_OK if the task was created successfully.
 */
esp_err_t mqttapp_start(void);

#ifdef __cplusplus
}
#endif

#endif /* MQTTAPP_H */