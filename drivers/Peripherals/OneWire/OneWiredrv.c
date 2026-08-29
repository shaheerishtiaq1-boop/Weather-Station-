/**
 * @file OneWiredrv.c
 * @brief Primitive-level single-wire GPIO driver.
 */

#include "OneWiredrv.h"

#include "esp_rom_sys.h"   // esp_rom_delay_us
#include "esp_timer.h"     // esp_timer_get_time
#include "esp_log.h"

static const char *TAG = "one_wire_drv";

esp_err_t one_wire_init(one_wire_t *ow, gpio_num_t gpio_num)
{
    if (ow == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << gpio_num),
        .mode         = GPIO_MODE_INPUT_OUTPUT_OD, /* open-drain: needed since
                                                        the sensor also drives
                                                        this line at times */
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };

    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "gpio_config failed: %s", esp_err_to_name(err));
        return err;
    }

    ow->gpio_num = gpio_num;

    /* Idle state: released (high via pull-up), input mode */
    gpio_set_level(gpio_num, 1);
    gpio_set_direction(gpio_num, GPIO_MODE_INPUT);

    return ESP_OK;
}

void one_wire_set_output(one_wire_t *ow)
{
    gpio_set_direction(ow->gpio_num, GPIO_MODE_OUTPUT_OD);
}

void one_wire_set_input(one_wire_t *ow)
{
    gpio_set_direction(ow->gpio_num, GPIO_MODE_INPUT);
}

void one_wire_write(one_wire_t *ow, int level)
{
    gpio_set_level(ow->gpio_num, level ? 1 : 0);
}

int one_wire_read(one_wire_t *ow)
{
    return gpio_get_level(ow->gpio_num);
}

void one_wire_delay_us(uint32_t us)
{
    esp_rom_delay_us(us);
}

esp_err_t one_wire_wait_for_level(one_wire_t *ow, int level, uint32_t timeout_us)
{
    uint32_t elapsed;
    return one_wire_wait_for_level_timed(ow, level, timeout_us, &elapsed);
}

esp_err_t one_wire_wait_for_level_timed(one_wire_t *ow, int level,
                                        uint32_t timeout_us,
                                        uint32_t *elapsed_us)
{
    int64_t start = esp_timer_get_time();

    while (gpio_get_level(ow->gpio_num) != level) {
        int64_t now = esp_timer_get_time();
        if ((now - start) > (int64_t)timeout_us) {
            if (elapsed_us) {
                *elapsed_us = (uint32_t)(now - start);
            }
            return ESP_ERR_TIMEOUT;
        }
    }

    if (elapsed_us) {
        *elapsed_us = (uint32_t)(esp_timer_get_time() - start);
    }
    return ESP_OK;
}