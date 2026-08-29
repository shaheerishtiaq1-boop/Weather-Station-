#include "relay.h"

#include "esp_check.h"

#define RELAY_TAG "RELAY"

esp_err_t relay_init(relay_t *relay, gpio_num_t gpioNum)
{
    ESP_RETURN_ON_FALSE(relay != NULL,
                        ESP_ERR_INVALID_ARG,
                        RELAY_TAG,
                        "Handle NULL");

    /* active-high: GPIO HIGH = relay energized/on. (Flipped from the
     * active-low default - this particular relay module turned out to
     * be active-high.) */
    return gpio_out_init(&relay->out, gpioNum, false);
}

esp_err_t relay_set(relay_t *relay, bool on)
{
    ESP_RETURN_ON_FALSE(relay != NULL,
                        ESP_ERR_INVALID_ARG,
                        RELAY_TAG,
                        "Handle NULL");

    return gpio_out_set(&relay->out, on);
}

bool relay_is_on(relay_t *relay)
{
    if (relay == NULL) {
        return false;
    }
    return gpio_out_get(&relay->out);
}