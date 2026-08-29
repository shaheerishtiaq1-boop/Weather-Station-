/**
 * @file wifi.c
 * @brief WiFi Station Manager.
 */

#include "wifi.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "esp_check.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#define WIFI_TAG "WIFI"

/* ========================= USER CONFIGURATION ========================= */

// #define WIFI_SSID      "StormFiber-2AB6-2.4G"
// #define WIFI_PASSWORD  "JXeY6dey"

#define WIFI_SSID      "embinx-Dev"
#define WIFI_PASSWORD  "netElastic@123$"

/* ==================================================================== */

#define WIFI_CONNECTED_BIT BIT0

static EventGroupHandle_t wifiEventGroup;

static bool wifiConnected = false;

static void wifi_event_handler(void *arg,
                               esp_event_base_t eventBase,
                               int32_t eventId,
                               void *eventData);

esp_err_t wifi_init(void)
{
    esp_err_t err;

    err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
        err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    ESP_RETURN_ON_ERROR(err,
                        WIFI_TAG,
                        "NVS initialization failed");

    wifiEventGroup = xEventGroupCreate();

    ESP_RETURN_ON_FALSE(wifiEventGroup != NULL,
                        ESP_ERR_NO_MEM,
                        WIFI_TAG,
                        "Event group creation failed");

    ESP_RETURN_ON_ERROR(
        esp_netif_init(),
        WIFI_TAG,
        "esp_netif_init failed");

    ESP_RETURN_ON_ERROR(
        esp_event_loop_create_default(),
        WIFI_TAG,
        "Event loop creation failed");

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_RETURN_ON_ERROR(
        esp_wifi_init(&cfg),
        WIFI_TAG,
        "WiFi initialization failed");

    ESP_RETURN_ON_ERROR(
        esp_event_handler_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &wifi_event_handler,
            NULL),
        WIFI_TAG,
        "WiFi event registration failed");

    ESP_RETURN_ON_ERROR(
        esp_event_handler_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            &wifi_event_handler,
            NULL),
        WIFI_TAG,
        "IP event registration failed");

    wifi_config_t wifiConfig = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    strcpy((char *)wifiConfig.sta.ssid,
           WIFI_SSID);

    strcpy((char *)wifiConfig.sta.password,
           WIFI_PASSWORD);

    ESP_RETURN_ON_ERROR(
        esp_wifi_set_mode(WIFI_MODE_STA),
        WIFI_TAG,
        "Failed to set station mode");

    ESP_RETURN_ON_ERROR(
        esp_wifi_set_config(WIFI_IF_STA,
                            &wifiConfig),
        WIFI_TAG,
        "Failed to set WiFi configuration");

    ESP_RETURN_ON_ERROR(
        esp_wifi_start(),
        WIFI_TAG,
        "Failed to start WiFi");

    ESP_LOGI(WIFI_TAG,
             "Connecting to \"%s\"...",
             WIFI_SSID);

    return ESP_OK;
}
static void wifi_event_handler(void *arg,
                               esp_event_base_t eventBase,
                               int32_t eventId,
                               void *eventData)
{
    if (eventBase == WIFI_EVENT)
    {
        switch (eventId)
        {
            case WIFI_EVENT_STA_START:

                esp_wifi_connect();

                break;

            case WIFI_EVENT_STA_DISCONNECTED:

                wifiConnected = false;

                xEventGroupClearBits(
                    wifiEventGroup,
                    WIFI_CONNECTED_BIT);

                ESP_LOGW(WIFI_TAG,
                         "Disconnected. Reconnecting...");

                esp_wifi_connect();

                break;

            default:

                break;
        }
    }

    if (eventBase == IP_EVENT &&
        eventId == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event =
            (ip_event_got_ip_t *)eventData;

        wifiConnected = true;

        xEventGroupSetBits(
            wifiEventGroup,
            WIFI_CONNECTED_BIT);

        ESP_LOGI(WIFI_TAG,
                 "Connected");

        ESP_LOGI(WIFI_TAG,
                 "IP Address: " IPSTR,
                 IP2STR(&event->ip_info.ip));
    }
}
bool wifi_is_connected(void)
{
    return wifiConnected;
}

void wifi_wait_connected(void)
{
    xEventGroupWaitBits(
        wifiEventGroup,
        WIFI_CONNECTED_BIT,
        pdFALSE,
        pdTRUE,
        portMAX_DELAY);
}