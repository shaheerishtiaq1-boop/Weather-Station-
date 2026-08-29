/**
 * @file gpio.h
 * @brief Generic digital output pin driver.
 *
 * Wraps a single GPIO configured as a push-pull output, with optional
 * active-low logic (common on relay modules, where a LOW signal energizes
 * the relay). mosfet.c and relay.c both build on this.
 */

#ifndef GPIO_H
#define GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "esp_err.h"
#include "driver/gpio.h"

/**
 * @brief Digital output pin handle.
 */
typedef struct
{
    gpio_num_t gpioNum;

    bool activeLow;

    bool isOn;

} gpio_out_t;

/**
 * @brief Configure a GPIO as a push-pull output and drive it to the
 *        "off" state.
 *
 * @param out        Pointer to handle to initialize.
 * @param gpioNum    GPIO to use.
 * @param activeLow  If true, a LOW signal means "on" (common for many
 *                   relay modules). If false, HIGH means "on" (typical
 *                   for logic-level MOSFET driver boards).
 *
 * @return ESP_OK on success.
 */
esp_err_t gpio_out_init(gpio_out_t *out,
                        gpio_num_t gpioNum,
                        bool activeLow);

/**
 * @brief Turn the output on or off.
 *
 * @param out  Handle.
 * @param on   true = on, false = off (activeLow handling is automatic).
 *
 * @return ESP_OK on success.
 */
esp_err_t gpio_out_set(gpio_out_t *out, bool on);

/**
 * @brief Returns the last commanded state (true = on).
 */
bool gpio_out_get(gpio_out_t *out);

#ifdef __cplusplus
}
#endif

#endif /* GPIO_H */