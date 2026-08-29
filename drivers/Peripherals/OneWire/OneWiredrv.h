/**
 * @file OneWiredrv.h
 * @brief Primitive-level single-wire GPIO driver.
 *
 * This driver exposes low-level bus primitives (direction switching, raw
 * level writes, precise delays, and level-with-timeout waits). Protocol
 * logic — start signals, bit/byte decoding, checksum verification — lives
 * in the sensor layer (e.g. DHT22.c), which composes these primitives.
 */

#ifndef ONEWIREDRV_H
#define ONEWIREDRV_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Handle representing a single-wire bus on one GPIO pin.
 */
typedef struct {
    gpio_num_t gpio_num;
} one_wire_t;

/**
 * @brief Initialize the GPIO pin used for the single-wire bus.
 *
 * Configures the pin as open-drain input/output with pull-up enabled,
 * and leaves it idling high (input mode).
 *
 * @param ow        Pointer to bus handle to initialize.
 * @param gpio_num  GPIO number the sensor's data line is connected to.
 * @return ESP_OK on success, esp_err_t error code otherwise.
 */
esp_err_t one_wire_init(one_wire_t *ow, gpio_num_t gpio_num);

/**
 * @brief Switch the bus pin to output mode (host drives the line).
 */
void one_wire_set_output(one_wire_t *ow);

/**
 * @brief Switch the bus pin to input mode (host releases the line;
 *        pull-up or sensor drives it).
 */
void one_wire_set_input(one_wire_t *ow);

/**
 * @brief Drive the bus pin to the given level. Only meaningful after
 *        one_wire_set_output().
 *
 * @param ow     Bus handle.
 * @param level  0 or 1.
 */
void one_wire_write(one_wire_t *ow, int level);

/**
 * @brief Read the current instantaneous level of the bus pin.
 *
 * @return 0 or 1.
 */
int one_wire_read(one_wire_t *ow);

/**
 * @brief Busy-wait for a precise number of microseconds.
 *
 * Thin wrapper around esp_rom_delay_us(), provided here so callers don't
 * need to include esp_rom_sys.h directly.
 */
void one_wire_delay_us(uint32_t us);

/**
 * @brief Block until the bus pin reaches the given level, or timeout.
 *
 * @param ow          Bus handle.
 * @param level       Level to wait for (0 or 1).
 * @param timeout_us  Maximum time to wait, in microseconds.
 * @return ESP_OK if the level was reached, ESP_ERR_TIMEOUT otherwise.
 */
esp_err_t one_wire_wait_for_level(one_wire_t *ow, int level, uint32_t timeout_us);

/**
 * @brief Same as one_wire_wait_for_level(), but also reports how long
 *        (in microseconds) the wait took. Used for bit decoding, where the
 *        duration of a high pulse encodes a 0 or 1.
 *
 * @param ow           Bus handle.
 * @param level        Level to wait for (0 or 1).
 * @param timeout_us   Maximum time to wait, in microseconds.
 * @param elapsed_us   Output: microseconds elapsed until the level was
 *                     reached (undefined on timeout).
 * @return ESP_OK if the level was reached, ESP_ERR_TIMEOUT otherwise.
 */
esp_err_t one_wire_wait_for_level_timed(one_wire_t *ow, int level,
                                        uint32_t timeout_us,
                                        uint32_t *elapsed_us);

#ifdef __cplusplus
}
#endif

#endif /* ONEWIREDRV_H */