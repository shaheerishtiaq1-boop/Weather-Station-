/**
 * @file mqttDrv.c
 * @brief Generic MQTT client driver.
 */

#include "mqttDrv.h"

#include "esp_check.h"
#include "esp_log.h"

#define MQTTDRV_TAG "MQTTDRV"

#define MQTTDRV_CONNECTED_BIT BIT0

static void mqtt_event_handler(void *handlerArgs,
                               esp_event_base_t base,
                               int32_t eventId,
                               void *eventData);

esp_err_t mqtt_drv_init(mqtt_drv_t *mqtt,
                        const char *uri,
                        const char *username,
                        const char *password)
{
    ESP_RETURN_ON_FALSE(mqtt != NULL,
                        ESP_ERR_INVALID_ARG,
                        MQTTDRV_TAG,
                        "Handle NULL");

    ESP_RETURN_ON_FALSE(uri != NULL,
                        ESP_ERR_INVALID_ARG,
                        MQTTDRV_TAG,
                        "URI NULL");

    mqtt->dataCallback = NULL;
    mqtt->userCtx = NULL;

    mqtt->eventGroup = xEventGroupCreate();

    ESP_RETURN_ON_FALSE(mqtt->eventGroup != NULL,
                        ESP_ERR_NO_MEM,
                        MQTTDRV_TAG,
                        "Event group creation failed");

    esp_mqtt_client_config_t mqttConfig = {
        .broker.address.uri = uri,
        .credentials.username = username,
        .credentials.authentication.password = password,
    };

    mqtt->clientHandle = esp_mqtt_client_init(&mqttConfig);

    ESP_RETURN_ON_FALSE(mqtt->clientHandle != NULL,
                        ESP_FAIL,
                        MQTTDRV_TAG,
                        "Client init failed");

    ESP_RETURN_ON_ERROR(
        esp_mqtt_client_register_event(mqtt->clientHandle,
                                       ESP_EVENT_ANY_ID,
                                       mqtt_event_handler,
                                       mqtt),
        MQTTDRV_TAG,
        "Event registration failed");

    ESP_RETURN_ON_ERROR(
        esp_mqtt_client_start(mqtt->clientHandle),
        MQTTDRV_TAG,
        "Client start failed");

    ESP_LOGI(MQTTDRV_TAG,
             "Connecting to \"%s\"...",
             uri);

    return ESP_OK;
}

esp_err_t mqtt_drv_deinit(mqtt_drv_t *mqtt)
{
    ESP_RETURN_ON_FALSE(mqtt != NULL,
                        ESP_ERR_INVALID_ARG,
                        MQTTDRV_TAG,
                        "Handle NULL");

    ESP_RETURN_ON_ERROR(
        esp_mqtt_client_stop(mqtt->clientHandle),
        MQTTDRV_TAG,
        "Client stop failed");

    ESP_RETURN_ON_ERROR(
        esp_mqtt_client_destroy(mqtt->clientHandle),
        MQTTDRV_TAG,
        "Client destroy failed");

    if (mqtt->eventGroup != NULL) {
        vEventGroupDelete(mqtt->eventGroup);
        mqtt->eventGroup = NULL;
    }

    return ESP_OK;
}

int mqtt_drv_publish(mqtt_drv_t *mqtt,
                     const char *topic,
                     const char *payload,
                     int qos,
                     bool retain)
{
    if (mqtt == NULL || topic == NULL || payload == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    return esp_mqtt_client_publish(mqtt->clientHandle,
                                   topic,
                                   payload,
                                   0, /* payload length: 0 = use strlen(payload) */
                                   qos,
                                   retain ? 1 : 0);
}

int mqtt_drv_subscribe(mqtt_drv_t *mqtt,
                       const char *topic,
                       int qos)
{
    if (mqtt == NULL || topic == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    return esp_mqtt_client_subscribe(mqtt->clientHandle, topic, qos);
}

void mqtt_drv_set_data_callback(mqtt_drv_t *mqtt,
                                mqtt_drv_data_cb_t callback,
                                void *userCtx)
{
    if (mqtt == NULL) {
        return;
    }

    mqtt->dataCallback = callback;
    mqtt->userCtx = userCtx;
}

bool mqtt_drv_is_connected(mqtt_drv_t *mqtt)
{
    if (mqtt == NULL || mqtt->eventGroup == NULL) {
        return false;
    }

    return (xEventGroupGetBits(mqtt->eventGroup) & MQTTDRV_CONNECTED_BIT) != 0;
}

void mqtt_drv_wait_connected(mqtt_drv_t *mqtt)
{
    if (mqtt == NULL || mqtt->eventGroup == NULL) {
        return;
    }

    xEventGroupWaitBits(mqtt->eventGroup,
                        MQTTDRV_CONNECTED_BIT,
                        pdFALSE,
                        pdTRUE,
                        portMAX_DELAY);
}

static void mqtt_event_handler(void *handlerArgs,
                               esp_event_base_t base,
                               int32_t eventId,
                               void *eventData)
{
    (void)base;

    mqtt_drv_t *mqtt = (mqtt_drv_t *)handlerArgs;
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)eventData;

    switch (eventId) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(MQTTDRV_TAG, "Connected");
            xEventGroupSetBits(mqtt->eventGroup, MQTTDRV_CONNECTED_BIT);
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(MQTTDRV_TAG, "Disconnected");
            xEventGroupClearBits(mqtt->eventGroup, MQTTDRV_CONNECTED_BIT);
            break;

        case MQTT_EVENT_DATA:
            if (mqtt->dataCallback != NULL) {
                mqtt->dataCallback(event->topic, event->topic_len,
                                   event->data, event->data_len,
                                   mqtt->userCtx);
            }
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(MQTTDRV_TAG, "MQTT error event");
            break;

        default:
            break;
    }
}