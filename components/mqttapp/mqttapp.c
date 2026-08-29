/**
 * @file mqttapp.c
 * @brief Application layer: reads DHT22 + BH1750, always shows the latest
 *        readings on the OLED, publishes telemetry to ThingsBoard, and
 *        listens for RPC calls from ThingsBoard to control the MOSFET
 *        and relay outputs.
 *
 * The OLED update does NOT wait for WiFi or MQTT - sensors are read and the
 * screen is refreshed every cycle regardless of connectivity. MQTT publish
 * is attempted opportunistically each cycle; if the client isn't connected,
 * the publish is simply skipped and the loop continues normally.
 *
 * RPC handling: ThingsBoard control widgets (e.g. a Switch) send an RPC
 * call over MQTT on topic "v1/devices/me/rpc/request/{requestId}" with a
 * JSON body like {"method":"setMosfet","params":true}. This module
 * subscribes to that topic once connected, parses incoming calls, drives
 * the corresponding output, and publishes an acknowledgement back on
 * "v1/devices/me/rpc/response/{requestId}" so the widget reflects the new
 * state.
 */

#include "mqttapp.h"

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_check.h"
#include "esp_log.h"
#include "cJSON.h"

#include "wifi.h"
#include "i2cdrv.h"
#include "DHT22.h"
#include "BH1750.h"
#include "ssd1306.h"
#include "mqttDrv.h"
#include "mosfet.h"
#include "relay.h"

#define MQTTAPP_TAG "MQTTAPP"

/* ========================= USER CONFIGURATION ========================= */

#define THINGSBOARD_MQTT_URI      "mqtt://mqtt.thingsboard.cloud:1883"
#define THINGSBOARD_ACCESS_TOKEN  "ssi4rwi9NoDKK8KrZ9XY"

#define DHT22_GPIO                GPIO_NUM_2

#define I2C_PORT                  I2C_NUM_0
#define I2C_SDA_GPIO              GPIO_NUM_8
#define I2C_SCL_GPIO              GPIO_NUM_9
#define I2C_FREQUENCY_HZ          400000

#define OLED_I2C_ADDRESS          0x3C

#define MOSFET_GPIO                GPIO_NUM_21
#define RELAY_GPIO                 GPIO_NUM_47

#define TELEMETRY_TOPIC           "v1/devices/me/telemetry"
#define RPC_REQUEST_TOPIC         "v1/devices/me/rpc/request/+"
#define RPC_REQUEST_PREFIX        "v1/devices/me/rpc/request/"
#define RPC_RESPONSE_PREFIX       "v1/devices/me/rpc/response/"

#define LOOP_INTERVAL_MS          1000  /* OLED + MQTT refresh cadence */
#define DHT22_READ_EVERY_N_LOOPS  3     /* DHT22 physically can't be read
                                          * faster than ~2s; reading it every
                                          * 3rd loop (~3s) stays safely above
                                          * that floor while BH1750 + display
                                          * + publish still run every 1s */

/* ==================================================================== */

/**
 * @brief Context passed to the MQTT data callback so it can reach the
 *        output handles and publish responses.
 */
typedef struct
{
    mosfet_t *mosfet;
    relay_t  *relay;
    mqtt_drv_t *mqtt;

} mqttapp_rpc_ctx_t;

static void mqttapp_task(void *arg);
static void mqttapp_update_display(ssd1306_t *oled, dht22_t *dht, bh1750_t *bh1750);
static void mqttapp_on_mqtt_data(const char *topic, int topicLen,
                                 const char *data, int dataLen,
                                 void *userCtx);

esp_err_t mqttapp_start(void)
{
    BaseType_t ok = xTaskCreate(mqttapp_task,
                                "mqttapp_task",
                                8192,
                                NULL,
                                5,
                                NULL);

    return (ok == pdPASS) ? ESP_OK : ESP_ERR_NO_MEM;
}

static void mqttapp_task(void *arg)
{
    (void)arg;

    /* --- Sensor + display + output setup: no network dependency at all --- */

    i2c_drv_t i2c;
    esp_err_t err = i2c_drv_init(&i2c, I2C_PORT, I2C_SDA_GPIO, I2C_SCL_GPIO, I2C_FREQUENCY_HZ);
    if (err != ESP_OK) {
        ESP_LOGE(MQTTAPP_TAG, "i2c_drv_init failed: %s", esp_err_to_name(err));
        vTaskDelete(NULL);
        return;
    }

    dht22_t dht;
    err = dht22_init(&dht, DHT22_GPIO);
    if (err != ESP_OK) {
        ESP_LOGE(MQTTAPP_TAG, "dht22_init failed: %s", esp_err_to_name(err));
        vTaskDelete(NULL);
        return;
    }

    bh1750_t bh1750;
    err = bh1750_init(&bh1750, &i2c);
    if (err != ESP_OK) {
        ESP_LOGE(MQTTAPP_TAG, "bh1750_init failed: %s", esp_err_to_name(err));
        vTaskDelete(NULL);
        return;
    }

    ssd1306_t oled;
    err = ssd1306_init(&oled, &i2c, OLED_I2C_ADDRESS);
    if (err != ESP_OK) {
        ESP_LOGE(MQTTAPP_TAG, "ssd1306_init failed: %s", esp_err_to_name(err));
        vTaskDelete(NULL);
        return;
    }

    mosfet_t mosfet;
    err = mosfet_init(&mosfet, MOSFET_GPIO);
    if (err != ESP_OK) {
        ESP_LOGE(MQTTAPP_TAG, "mosfet_init failed: %s", esp_err_to_name(err));
        vTaskDelete(NULL);
        return;
    }

    relay_t relay;
    err = relay_init(&relay, RELAY_GPIO);
    if (err != ESP_OK) {
        ESP_LOGE(MQTTAPP_TAG, "relay_init failed: %s", esp_err_to_name(err));
        vTaskDelete(NULL);
        return;
    }

    /* --- MQTT setup: started, but never blocks this task.
     * esp-mqtt retries connecting on its own in the background, even if
     * WiFi isn't up yet at this exact moment. --- */

    mqtt_drv_t mqtt;
    bool mqttReady = (mqtt_drv_init(&mqtt, THINGSBOARD_MQTT_URI,
                                    THINGSBOARD_ACCESS_TOKEN, NULL) == ESP_OK);
    if (!mqttReady) {
        ESP_LOGW(MQTTAPP_TAG, "mqtt_drv_init failed - will keep updating OLED without publishing/control");
    }

    static mqttapp_rpc_ctx_t rpcCtx; /* static: outlives this function, callback fires later */
    rpcCtx.mosfet = &mosfet;
    rpcCtx.relay = &relay;
    rpcCtx.mqtt = &mqtt;

    if (mqttReady) {
        mqtt_drv_set_data_callback(&mqtt, mqttapp_on_mqtt_data, &rpcCtx);
    }

    bool subscribed = false;
    uint32_t loopCount = 0;
    char payload[128];

    while (1) {
        /* DHT22 can't physically be read faster than ~2s - only read it
         * every DHT22_READ_EVERY_N_LOOPS ticks (~3s at 1s/loop). BH1750
         * and everything else runs every loop (~1s), always using the
         * latest cached DHT22 values in between actual reads. */
        if (loopCount % DHT22_READ_EVERY_N_LOOPS == 0) {
            esp_err_t dhtErr = dht22_read(&dht);
            if (dhtErr != ESP_OK) {
                ESP_LOGW(MQTTAPP_TAG, "dht22_read failed: %s (using last known values)",
                         esp_err_to_name(dhtErr));
            }
        }
        loopCount++;

        esp_err_t bhErr = bh1750_read(&bh1750);
        if (bhErr != ESP_OK) {
            ESP_LOGW(MQTTAPP_TAG, "bh1750_read failed: %s (using last known values)",
                     esp_err_to_name(bhErr));
        }

        /* Always update the OLED, regardless of WiFi/MQTT state */
        mqttapp_update_display(&oled, &dht, &bh1750);

        if (mqttReady && mqtt_drv_is_connected(&mqtt)) {
            /* Subscribe to RPC requests once, the first time we see the
             * connection come up (also re-subscribes if it reconnects
             * after a drop, since `subscribed` only tracks this task's
             * lifetime, not the broker session). */
            if (!subscribed) {
                int subId = mqtt_drv_subscribe(&mqtt, RPC_REQUEST_TOPIC, 1);
                if (subId >= 0) {
                    ESP_LOGI(MQTTAPP_TAG, "Subscribed to RPC requests");
                    subscribed = true;
                } else {
                    ESP_LOGW(MQTTAPP_TAG, "RPC subscribe failed, will retry next cycle");
                }
            }

            int len = snprintf(payload, sizeof(payload),
                               "{\"temperature\":%.1f,\"humidity\":%.1f,\"lux\":%.2f,"
                               "\"mosfetState\":%s,\"relayState\":%s}",
                               dht22_get_temperature(&dht),
                               dht22_get_humidity(&dht),
                               bh1750_get_lux(&bh1750),
                               mosfet_is_on(&mosfet) ? "true" : "false",
                               relay_is_on(&relay) ? "true" : "false");

            if (len > 0 && len < (int)sizeof(payload)) {
                int msgId = mqtt_drv_publish(&mqtt, TELEMETRY_TOPIC, payload, 1, false);
                if (msgId < 0) {
                    ESP_LOGW(MQTTAPP_TAG, "publish failed");
                } else {
                    ESP_LOGI(MQTTAPP_TAG, "Published: %s", payload);
                }
            }
        } else {
            subscribed = false; /* connection dropped - re-subscribe once back up */
            ESP_LOGW(MQTTAPP_TAG, "MQTT not connected - skipping publish (OLED still updated)");
        }

        vTaskDelay(pdMS_TO_TICKS(LOOP_INTERVAL_MS));
    }
}

static void mqttapp_update_display(ssd1306_t *oled, dht22_t *dht, bh1750_t *bh1750)
{
    /* Cycles which reading is shown big on screen: 0=temp, 1=humidity,
     * 2=lux. This function is called every LOOP_INTERVAL_MS (~1s) so the
     * underlying numbers are always fresh, but we only advance which
     * value is *shown* every DISPLAY_SWITCH_EVERY_N_CALLS calls (~3s) -
     * switching every 1s would be too fast to actually read. */
    #define DISPLAY_SWITCH_EVERY_N_CALLS 3

    static uint8_t displayIndex = 0;
    static uint32_t callCount = 0;

    if (callCount % DISPLAY_SWITCH_EVERY_N_CALLS == 0) {
        displayIndex = (uint8_t)((displayIndex + 1) % 3);
    }
    callCount++;

    char label[16];
    char value[16];

    switch (displayIndex) {
        case 0:
            snprintf(label, sizeof(label), "Temperature");
            snprintf(value, sizeof(value), "%.1f C", dht22_get_temperature(dht));
            break;

        case 1:
            snprintf(label, sizeof(label), "Humidity");
            snprintf(value, sizeof(value), "%.1f %%", dht22_get_humidity(dht));
            break;

        default:
            snprintf(label, sizeof(label), "Light (lux)");
            snprintf(value, sizeof(value), "%.1f", bh1750_get_lux(bh1750));
            break;
    }

    ssd1306_clear(oled);

    /* Small label, centered, near the top */
    uint8_t labelWidth = (uint8_t)(strlen(label) * 6);
    uint8_t labelX = (labelWidth < SSD1306_WIDTH) ? (uint8_t)((SSD1306_WIDTH - labelWidth) / 2) : 0;
    ssd1306_set_cursor(oled, labelX, 0);
    ssd1306_print(oled, label);

    /* Big value, scale 3, centered, filling most of the screen */
    const uint8_t scale = 3;
    uint8_t valueWidth = (uint8_t)(strlen(value) * 6 * scale);
    uint8_t valueX = (valueWidth < SSD1306_WIDTH) ? (uint8_t)((SSD1306_WIDTH - valueWidth) / 2) : 0;
    ssd1306_set_cursor(oled, valueX, 24);
    ssd1306_print_scaled(oled, value, scale);

    ssd1306_display(oled);
}

/**
 * @brief Handles incoming MQTT messages - specifically, ThingsBoard RPC
 *        requests aimed at controlling the MOSFET or relay.
 *
 * Expected payload: {"method":"setMosfet","params":true}
 *               or:  {"method":"setRelay","params":false}
 *
 * Responds on the matching rpc/response topic with the new state, so
 * ThingsBoard Switch widgets reflect the actual result.
 */
static void mqttapp_on_mqtt_data(const char *topic, int topicLen,
                                 const char *data, int dataLen,
                                 void *userCtx)
{
    mqttapp_rpc_ctx_t *ctx = (mqttapp_rpc_ctx_t *)userCtx;

    if (ctx == NULL || topic == NULL || data == NULL) {
        return;
    }

    /* Topic/data from the MQTT event aren't guaranteed null-terminated -
     * copy into local buffers first. */
    char topicBuf[80];
    char dataBuf[128];

    if (topicLen <= 0 || topicLen >= (int)sizeof(topicBuf) ||
        dataLen <= 0  || dataLen  >= (int)sizeof(dataBuf)) {
        ESP_LOGW(MQTTAPP_TAG, "RPC message too large or empty, ignoring");
        return;
    }

    memcpy(topicBuf, topic, (size_t)topicLen);
    topicBuf[topicLen] = '\0';

    memcpy(dataBuf, data, (size_t)dataLen);
    dataBuf[dataLen] = '\0';

    if (strncmp(topicBuf, RPC_REQUEST_PREFIX, strlen(RPC_REQUEST_PREFIX)) != 0) {
        return; /* not an RPC request we care about */
    }

    const char *requestId = topicBuf + strlen(RPC_REQUEST_PREFIX);

    cJSON *root = cJSON_Parse(dataBuf);
    if (root == NULL) {
        ESP_LOGW(MQTTAPP_TAG, "RPC payload is not valid JSON: %s", dataBuf);
        return;
    }

    const cJSON *methodItem = cJSON_GetObjectItemCaseSensitive(root, "method");

    if (!cJSON_IsString(methodItem)) {
        ESP_LOGW(MQTTAPP_TAG, "RPC payload missing string 'method'");
        cJSON_Delete(root);
        return;
    }

    char responseTopic[96];
    snprintf(responseTopic, sizeof(responseTopic), "%s%s", RPC_RESPONSE_PREFIX, requestId);

    /* --- "get state" queries: dashboard asks for current state, e.g. on
     * page load. Answer directly from the last known pin state - no pin
     * change happens here. --- */
    if (strcmp(methodItem->valuestring, "getMosfetState") == 0) {
        bool state = mosfet_is_on(ctx->mosfet);
        mqtt_drv_publish(ctx->mqtt, responseTopic, state ? "true" : "false", 1, false);
        ESP_LOGI(MQTTAPP_TAG, "RPC: get mosfet state -> %s", state ? "ON" : "OFF");
        cJSON_Delete(root);
        return;
    }

    if (strcmp(methodItem->valuestring, "getRelayState") == 0) {
        bool state = relay_is_on(ctx->relay);
        mqtt_drv_publish(ctx->mqtt, responseTopic, state ? "true" : "false", 1, false);
        ESP_LOGI(MQTTAPP_TAG, "RPC: get relay state -> %s", state ? "ON" : "OFF");
        cJSON_Delete(root);
        return;
    }

    /* --- "set state" commands: dashboard toggle actually changing a pin --- */
    const cJSON *paramsItem = cJSON_GetObjectItemCaseSensitive(root, "params");

    bool handled = false;
    bool newState = false;

    if (cJSON_IsBool(paramsItem)) {
        newState = cJSON_IsTrue(paramsItem);

        if (strcmp(methodItem->valuestring, "setMosfet") == 0) {
            mosfet_set(ctx->mosfet, newState);
            handled = true;
            ESP_LOGI(MQTTAPP_TAG, "RPC: mosfet -> %s", newState ? "ON" : "OFF");
        } else if (strcmp(methodItem->valuestring, "setRelay") == 0) {
            relay_set(ctx->relay, newState);
            handled = true;
            ESP_LOGI(MQTTAPP_TAG, "RPC: relay -> %s", newState ? "ON" : "OFF");
        } else {
            ESP_LOGW(MQTTAPP_TAG, "Unknown RPC method: %s", methodItem->valuestring);
        }
    } else {
        ESP_LOGW(MQTTAPP_TAG, "RPC 'set' call missing boolean 'params'");
    }

    if (handled) {
        char responseBody[16];
        snprintf(responseBody, sizeof(responseBody), "%s", newState ? "true" : "false");

        mqtt_drv_publish(ctx->mqtt, responseTopic, responseBody, 1, false);

        /* Also push the new state as telemetry right away, rather than
         * waiting for the next periodic publish cycle - keeps the
         * dashboard's switch position in sync immediately. */
        char statePayload[48];
        const char *stateKey = (strcmp(methodItem->valuestring, "setMosfet") == 0)
                                ? "mosfetState" : "relayState";
        snprintf(statePayload, sizeof(statePayload), "{\"%s\":%s}", stateKey, responseBody);
        mqtt_drv_publish(ctx->mqtt, TELEMETRY_TOPIC, statePayload, 1, false);
    }

    cJSON_Delete(root);
}