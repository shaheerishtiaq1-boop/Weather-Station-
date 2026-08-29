/**
 * @file mqttDrv.h
 * @brief Generic MQTT client driver built on ESP-IDF's esp-mqtt component.
 *
 * This driver provides primitive MQTT operations (connect, publish,
 * subscribe, and a data-received callback). It has no knowledge of any
 * specific broker or payload format - that belongs in the application
 * layer (e.g. mqttapp), which composes these primitives to talk to
 * ThingsBoard or any other broker.
 */

#ifndef MQTTDRV_H
#define MQTTDRV_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "mqtt_client.h"

/**
 * @brief Callback invoked whenever data arrives on a subscribed topic.
 *
 * @param topic      Topic the message was received on (not null-terminated
 *                    guaranteed - use topicLength).
 * @param topicLength Length of the topic string.
 * @param data       Message payload (not null-terminated guaranteed - use
 *                    dataLength).
 * @param dataLength Length of the payload.
 * @param userCtx    User context pointer passed to mqtt_drv_set_data_callback().
 */
typedef void (*mqtt_drv_data_cb_t)(const char *topic, int topicLength,
                                   const char *data, int dataLength,
                                   void *userCtx);

/**
 * @brief MQTT driver handle.
 */
typedef struct
{
    esp_mqtt_client_handle_t clientHandle;

    EventGroupHandle_t eventGroup;

    mqtt_drv_data_cb_t dataCallback;

    void *userCtx;

} mqtt_drv_t;

/**
 * @brief Initialize and start the MQTT client, connecting to the given broker.
 *
 * Connection happens asynchronously in the background; use
 * mqtt_drv_is_connected() or mqtt_drv_wait_connected() to check status.
 *
 * @param mqtt      Driver handle.
 * @param uri       Broker URI, e.g. "mqtt://mqtt.thingsboard.cloud:1883".
 * @param username  MQTT username (for ThingsBoard: the device access token).
 *                  Pass NULL if not needed.
 * @param password  MQTT password. Pass NULL if not needed.
 *
 * @return ESP_OK on success.
 */
esp_err_t mqtt_drv_init(mqtt_drv_t *mqtt,
                        const char *uri,
                        const char *username,
                        const char *password);

/**
 * @brief Stop and destroy the MQTT client.
 *
 * @param mqtt Driver handle.
 *
 * @return ESP_OK on success.
 */
esp_err_t mqtt_drv_deinit(mqtt_drv_t *mqtt);

/**
 * @brief Publish a message.
 *
 * @param mqtt     Driver handle.
 * @param topic    Topic to publish to.
 * @param payload  Null-terminated payload string.
 * @param qos      QoS level (0, 1, or 2).
 * @param retain   Whether the broker should retain this message.
 *
 * @return Message ID on success (>= 0), or a negative esp_err_t-style
 *         error code on failure.
 */
int mqtt_drv_publish(mqtt_drv_t *mqtt,
                     const char *topic,
                     const char *payload,
                     int qos,
                     bool retain);

/**
 * @brief Subscribe to a topic.
 *
 * @param mqtt   Driver handle.
 * @param topic  Topic to subscribe to.
 * @param qos    QoS level (0, 1, or 2).
 *
 * @return Message ID on success (>= 0), or a negative value on failure.
 */
int mqtt_drv_subscribe(mqtt_drv_t *mqtt,
                       const char *topic,
                       int qos);

/**
 * @brief Register a callback for incoming subscribed messages.
 *
 * @param mqtt      Driver handle.
 * @param callback  Function to call when data arrives.
 * @param userCtx   Opaque pointer passed back to the callback.
 */
void mqtt_drv_set_data_callback(mqtt_drv_t *mqtt,
                                mqtt_drv_data_cb_t callback,
                                void *userCtx);

/**
 * @brief Returns whether the MQTT client is currently connected to the broker.
 */
bool mqtt_drv_is_connected(mqtt_drv_t *mqtt);

/**
 * @brief Block until the MQTT client connects to the broker.
 */
void mqtt_drv_wait_connected(mqtt_drv_t *mqtt);

#ifdef __cplusplus
}
#endif

#endif /* MQTTDRV_H */