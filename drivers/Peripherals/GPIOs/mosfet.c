#include "mosfet.h"

#include "esp_check.h"

#define MOSFET_TAG "MOSFET"

esp_err_t mosfet_init(mosfet_t *mosfet, gpio_num_t gpioNum)
{
    ESP_RETURN_ON_FALSE(mosfet != NULL,
                        ESP_ERR_INVALID_ARG,
                        MOSFET_TAG,
                        "Handle NULL");

    /* active-high: GPIO HIGH = MOSFET on. Flip to `true` if your board
     * is wired active-low instead. */
    return gpio_out_init(&mosfet->out, gpioNum, false);
}

esp_err_t mosfet_set(mosfet_t *mosfet, bool on)
{
    ESP_RETURN_ON_FALSE(mosfet != NULL,
                        ESP_ERR_INVALID_ARG,
                        MOSFET_TAG,
                        "Handle NULL");

    return gpio_out_set(&mosfet->out, on);
}

bool mosfet_is_on(mosfet_t *mosfet)
{
    if (mosfet == NULL) {
        return false;
    }
    return gpio_out_get(&mosfet->out);
}