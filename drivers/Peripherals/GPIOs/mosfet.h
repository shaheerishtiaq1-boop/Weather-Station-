#ifndef MOSFET_H
#define MOSFET_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "esp_err.h"
#include "gpio.h"

/**
 * @brief MOSFET control handle.
 */
typedef struct
{
    gpio_out_t out;

} mosfet_t;

/**
 * @brief Initialize a MOSFET control pin.
 *
 * Assumes an active-high logic-level MOSFET driver board (GPIO HIGH turns
 * the MOSFET on) - the common case. If your board is wired the opposite
 * way, edit the `false` passed to gpio_out_init() in mosfet.c to `true`.
 *
 * @param mosfet   Handle to initialize.
 * @param gpioNum  GPIO connected to the MOSFET gate driver input.
 *
 * @return ESP_OK on success.
 */
esp_err_t mosfet_init(mosfet_t *mosfet, gpio_num_t gpioNum);

/**
 * @brief Turn the MOSFET on or off.
 */
esp_err_t mosfet_set(mosfet_t *mosfet, bool on);

/**
 * @brief Returns the last commanded state (true = on).
 */
bool mosfet_is_on(mosfet_t *mosfet);

#ifdef __cplusplus
}
#endif

#endif /* MOSFET_H */