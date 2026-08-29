/**
 * @file wifi.h
 * @brief WiFi Station Manager.
 *
 * Handles WiFi initialization, connection, automatic reconnection,
 * and provides helper functions to check or wait for connectivity.
 *
 * This module is independent of MQTT and application logic.
 */

#ifndef WIFI_H
#define WIFI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "esp_err.h"

/**
 * @brief Initialize the WiFi Station and connect to the configured AP.
 *
 * @return ESP_OK on success.
 */
esp_err_t wifi_init(void);

/**
 * @brief Returns whether WiFi is currently connected.
 *
 * @return true if connected.
 * @return false otherwise.
 */
bool wifi_is_connected(void);

/**
 * @brief Block until WiFi is connected.
 */
void wifi_wait_connected(void);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_H */