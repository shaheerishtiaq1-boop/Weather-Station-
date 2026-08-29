/**
 * @file gpio.c
 * @brief Generic digital output pin driver.
 */

#include "gpio.h"

#include "esp_check.h"
#include "esp_log.h"

#define GPIO_OUT_TAG "GPIO_OUT"

esp_err_t gpio_out_init(gpio_out_t *out,
                        gpio_num_t gpioNum,
                        bool activeLow)
{
    ESP_RETURN_ON_FALSE(out != NULL,
                        ESP_ERR_INVALID_ARG,
                        GPIO_OUT_TAG,
                        "Handle NULL");

    gpio_config_t ioConfig = {
        .pin_bit_mask = (1ULL << gpioNum),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };

    ESP_RETURN_ON_ERROR(
        gpio_config(&ioConfig),
        GPIO_OUT_TAG,
        "gpio_config failed");

    out->gpioNum = gpioNum;
    out->activeLow = activeLow;
    out->isOn = false;

    /* Drive to the "off" state immediately, so nothing floats on or
     * energizes unexpectedly at boot. */
    return gpio_out_set(out, false);
}

esp_err_t gpio_out_set(gpio_out_t *out, bool on)
{
    ESP_RETURN_ON_FALSE(out != NULL,
                        ESP_ERR_INVALID_ARG,
                        GPIO_OUT_TAG,
                        "Handle NULL");

    /* activeLow inverts the physical signal, not the logical "on" concept */
    int level = out->activeLow ? (on ? 0 : 1) : (on ? 1 : 0);

    ESP_RETURN_ON_ERROR(
        gpio_set_level(out->gpioNum, level),
        GPIO_OUT_TAG,
        "gpio_set_level failed");

    out->isOn = on;

    return ESP_OK;
}

bool gpio_out_get(gpio_out_t *out)
{
    if (out == NULL) {
        return false;
    }
    return out->isOn;
}