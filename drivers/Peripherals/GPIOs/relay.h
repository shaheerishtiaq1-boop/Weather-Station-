#ifndef RELAY_H
#define RELAY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "esp_err.h"
#include "gpio.h"

/**
 * @brief Relay control handle.
 */
typedef struct
{
    gpio_out_t out;

} relay_t;

/**
 * @brief Initialize a relay control pin.
 *
 * Assumes an active-low relay module (GPIO LOW energizes the relay) -
 * the common case for cheap 1/2/4/8-channel relay boards. If your module
 * is active-high instead, edit the `true` passed to gpio_out_init() in
 * relay.c to `false`.
 *
 * @param relay    Handle to initialize.
 * @param gpioNum  GPIO connected to the relay module's IN pin.
 *
 * @return ESP_OK on success.
 */
esp_err_t relay_init(relay_t *relay, gpio_num_t gpioNum);

/**
 * @brief Turn the relay on (energized) or off.
 */
esp_err_t relay_set(relay_t *relay, bool on);

/**
 * @brief Returns the last commanded state (true = on).
 */
bool relay_is_on(relay_t *relay);

#ifdef __cplusplus
}
#endif

#endif /* RELAY_H */