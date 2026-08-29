#ifndef BH1750_H
#define BH1750_H

#ifdef __cplusplus
extern "C" {
#endif

#include "i2cdrv.h"

/**
 * @brief BH1750 sensor handle.
 *
 * Stores the I2C device handle and the latest measured
 * light intensity.
 */
typedef struct
{
    i2c_device_t device;

    float lux;

} bh1750_t;

/**
 * @brief Initialize the BH1750 sensor.
 *
 * Registers the sensor on an already initialized I2C bus,
 * powers it on, resets it, and configures Continuous High
 * Resolution Mode.
 *
 * @param bh1750 Pointer to sensor handle.
 * @param i2c Pointer to initialized I2C bus.
 *
 * @return ESP_OK on success.
 */
esp_err_t bh1750_init(bh1750_t *bh1750,
                      i2c_drv_t *i2c);

/**
 * @brief Read the current light intensity.
 *
 * Reads the latest measurement from the sensor and updates
 * the stored lux value.
 *
 * @param bh1750 Pointer to sensor handle.
 *
 * @return ESP_OK on success.
 */
esp_err_t bh1750_read(bh1750_t *bh1750);

/**
 * @brief Get the latest measured lux value.
 *
 * @param bh1750 Pointer to sensor handle.
 *
 * @return Current light intensity in lux.
 */
float bh1750_get_lux(bh1750_t *bh1750);

/**
 * @brief Convenience one-call starter.
 *
 * Creates a background task that periodically reads
 * the BH1750 and logs the measured lux.
 *
 * @param i2c Pointer to initialized I2C bus.
 *
 * @return ESP_OK if task creation succeeded.
 */
esp_err_t bh1750_start(i2c_drv_t *i2c);

#ifdef __cplusplus
}
#endif

#endif /* BH1750_H */