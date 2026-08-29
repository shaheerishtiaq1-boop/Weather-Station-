/**
 * @file i2cdrv.c
 * @brief Generic I2C master driver.
 */

#include "i2cdrv.h"

#include "esp_check.h"
#include "esp_log.h"

#define I2CDRV_TAG "I2CDRV"

#define I2CDRV_TIMEOUT_MS 1000

esp_err_t i2c_drv_init(i2c_drv_t *i2c,
                       i2c_port_num_t port,
                       gpio_num_t sda,
                       gpio_num_t scl,
                       uint32_t frequency)
{
    ESP_RETURN_ON_FALSE(i2c != NULL,
                        ESP_ERR_INVALID_ARG,
                        I2CDRV_TAG,
                        "Handle NULL");

    i2c_master_bus_config_t busConfig = {
        .i2c_port = port,
        .sda_io_num = sda,
        .scl_io_num = scl,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ESP_RETURN_ON_ERROR(
        i2c_new_master_bus(&busConfig,
                           &i2c->busHandle),
        I2CDRV_TAG,
        "Bus initialization failed");

    i2c->frequency = frequency;

    return ESP_OK;
}

esp_err_t i2c_drv_deinit(i2c_drv_t *i2c)
{
    ESP_RETURN_ON_FALSE(i2c != NULL,
                        ESP_ERR_INVALID_ARG,
                        I2CDRV_TAG,
                        "Handle NULL");

    ESP_RETURN_ON_ERROR(
        i2c_del_master_bus(i2c->busHandle),
        I2CDRV_TAG,
        "Bus deinitialization failed");

    return ESP_OK;
}

esp_err_t i2c_drv_add_device(i2c_drv_t *i2c,
                             i2c_device_t *device,
                             uint16_t address)
{
    ESP_RETURN_ON_FALSE(i2c != NULL,
                        ESP_ERR_INVALID_ARG,
                        I2CDRV_TAG,
                        "Bus NULL");

    ESP_RETURN_ON_FALSE(device != NULL,
                        ESP_ERR_INVALID_ARG,
                        I2CDRV_TAG,
                        "Device NULL");

    i2c_device_config_t deviceConfig = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = address,
        .scl_speed_hz = i2c->frequency,
    };

    ESP_RETURN_ON_ERROR(
        i2c_master_bus_add_device(i2c->busHandle,
                                  &deviceConfig,
                                  &device->deviceHandle),
        I2CDRV_TAG,
        "Device add failed");

    return ESP_OK;
}

esp_err_t i2c_drv_remove_device(i2c_device_t *device)
{
    ESP_RETURN_ON_FALSE(device != NULL,
                        ESP_ERR_INVALID_ARG,
                        I2CDRV_TAG,
                        "Device NULL");

    ESP_RETURN_ON_ERROR(
        i2c_master_bus_rm_device(device->deviceHandle),
        I2CDRV_TAG,
        "Remove device failed");

    return ESP_OK;
}

esp_err_t i2c_drv_write(i2c_device_t *device,
                        const uint8_t *data,
                        size_t length)
{
    ESP_RETURN_ON_FALSE(device != NULL,
                        ESP_ERR_INVALID_ARG,
                        I2CDRV_TAG,
                        "Device NULL");

    return i2c_master_transmit(device->deviceHandle,
                               data,
                               length,
                               I2CDRV_TIMEOUT_MS);
}

esp_err_t i2c_drv_read(i2c_device_t *device,
                       uint8_t *data,
                       size_t length)
{
    ESP_RETURN_ON_FALSE(device != NULL,
                        ESP_ERR_INVALID_ARG,
                        I2CDRV_TAG,
                        "Device NULL");

    return i2c_master_receive(device->deviceHandle,
                              data,
                              length,
                              I2CDRV_TIMEOUT_MS);
}

esp_err_t i2c_drv_write_read(i2c_device_t *device,
                             const uint8_t *txData,
                             size_t txLength,
                             uint8_t *rxData,
                             size_t rxLength)
{
    ESP_RETURN_ON_FALSE(device != NULL,
                        ESP_ERR_INVALID_ARG,
                        I2CDRV_TAG,
                        "Device NULL");

    return i2c_master_transmit_receive(device->deviceHandle,
                                       txData,
                                       txLength,
                                       rxData,
                                       rxLength,
                                       I2CDRV_TIMEOUT_MS);
}