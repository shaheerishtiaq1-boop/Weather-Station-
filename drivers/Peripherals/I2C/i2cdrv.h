/**
 * @file i2cdrv.h
 * @brief Generic I2C master driver built on ESP-IDF's new I2C Master Bus API.
 *
 * This driver provides primitive I2C operations (bus initialization,
 * device registration, transmit, receive, and combined write/read).
 * Protocol logic belongs in higher-level device drivers (e.g. BH1750,
 * SSD1306), which compose these primitives.
 */

#ifndef I2CDRV_H
#define I2CDRV_H

#ifdef __cplusplus
extern "C" {
#endif

#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_err.h"

typedef struct
{
    i2c_master_bus_handle_t busHandle;

    uint32_t frequency;

} i2c_drv_t;

typedef struct
{
    i2c_master_dev_handle_t deviceHandle;

} i2c_device_t;

/**
 * @brief Initialize an I2C master bus.
 *
 * @param i2c        Driver handle.
 * @param port       I2C port.
 * @param sda        SDA GPIO.
 * @param scl        SCL GPIO.
 * @param frequency  Bus clock frequency.
 *
 * @return ESP_OK on success.
 */
esp_err_t i2c_drv_init(i2c_drv_t *i2c,
                       i2c_port_num_t port,
                       gpio_num_t sda,
                       gpio_num_t scl,
                       uint32_t frequency);

/**
 * @brief Deinitialize the I2C master bus.
 *
 * @param i2c Driver handle.
 *
 * @return ESP_OK on success.
 */
esp_err_t i2c_drv_deinit(i2c_drv_t *i2c);

/**
 * @brief Register a device on the I2C bus.
 *
 * @param i2c      Bus handle.
 * @param device   Device handle.
 * @param address  7-bit I2C address.
 *
 * @return ESP_OK on success.
 */
esp_err_t i2c_drv_add_device(i2c_drv_t *i2c,
                             i2c_device_t *device,
                             uint16_t address);

/**
 * @brief Remove a device from the I2C bus.
 *
 * @param device Device handle.
 *
 * @return ESP_OK on success.
 */
esp_err_t i2c_drv_remove_device(i2c_device_t *device);

/**
 * @brief Write bytes to an I2C device.
 */
esp_err_t i2c_drv_write(i2c_device_t *device,
                        const uint8_t *data,
                        size_t length);

/**
 * @brief Read bytes from an I2C device.
 */
esp_err_t i2c_drv_read(i2c_device_t *device,
                       uint8_t *data,
                       size_t length);

/**
 * @brief Perform a write followed by a read transaction.
 */
esp_err_t i2c_drv_write_read(i2c_device_t *device,
                             const uint8_t *txData,
                             size_t txLength,
                             uint8_t *rxData,
                             size_t rxLength);

#ifdef __cplusplus
}
#endif

#endif