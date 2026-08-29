#include "BH1750.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_check.h"
#include "esp_log.h"

#define BH1750_TAG "BH1750"

#define BH1750_I2C_ADDRESS            0x23

#define BH1750_POWER_DOWN             0x00
#define BH1750_POWER_ON               0x01
#define BH1750_RESET                  0x07

#define BH1750_CONTINUOUS_HIGH_RES    0x10

#define BH1750_CONVERSION_TIME_MS     180
#define BH1750_READ_PERIOD_MS         1000

static esp_err_t bh1750_power_on(bh1750_t *bh1750);
static esp_err_t bh1750_reset(bh1750_t *bh1750);
static esp_err_t bh1750_set_mode(bh1750_t *bh1750);

static void bh1750_task(void *arg);

esp_err_t bh1750_init(bh1750_t *bh1750,
                      i2c_drv_t *i2c)
{
    ESP_RETURN_ON_FALSE(bh1750 != NULL,
                        ESP_ERR_INVALID_ARG,
                        BH1750_TAG,
                        "Handle NULL");

    ESP_RETURN_ON_FALSE(i2c != NULL,
                        ESP_ERR_INVALID_ARG,
                        BH1750_TAG,
                        "I2C Handle NULL");

    bh1750->lux = 0.0f;

    ESP_RETURN_ON_ERROR(
        i2c_drv_add_device(i2c,
                           &bh1750->device,
                           BH1750_I2C_ADDRESS),
        BH1750_TAG,
        "Failed to add device");

    ESP_RETURN_ON_ERROR(
        bh1750_power_on(bh1750),
        BH1750_TAG,
        "Power ON failed");

    ESP_RETURN_ON_ERROR(
        bh1750_reset(bh1750),
        BH1750_TAG,
        "Reset failed");

    ESP_RETURN_ON_ERROR(
        bh1750_set_mode(bh1750),
        BH1750_TAG,
        "Mode configuration failed");

    vTaskDelay(pdMS_TO_TICKS(BH1750_CONVERSION_TIME_MS));

    return ESP_OK;
}

static esp_err_t bh1750_power_on(bh1750_t *bh1750)
{
    uint8_t command = BH1750_POWER_ON;

    return i2c_drv_write(&bh1750->device,
                         &command,
                         sizeof(command));
}

static esp_err_t bh1750_reset(bh1750_t *bh1750)
{
    uint8_t command = BH1750_RESET;

    return i2c_drv_write(&bh1750->device,
                         &command,
                         sizeof(command));
}

static esp_err_t bh1750_set_mode(bh1750_t *bh1750)
{
    uint8_t command = BH1750_CONTINUOUS_HIGH_RES;

    return i2c_drv_write(&bh1750->device,
                         &command,
                         sizeof(command));
}esp_err_t bh1750_read(bh1750_t *bh1750)
{
    ESP_RETURN_ON_FALSE(bh1750 != NULL,
                        ESP_ERR_INVALID_ARG,
                        BH1750_TAG,
                        "Handle NULL");

    uint8_t data[2] = {0};

    ESP_RETURN_ON_ERROR(
        i2c_drv_read(&bh1750->device,
                     data,
                     sizeof(data)),
        BH1750_TAG,
        "Read failed");

    uint16_t raw = ((uint16_t)data[0] << 8) | data[1];

    /*
     * BH1750 datasheet:
     * Lux = Raw / 1.2
     */
    bh1750->lux = (float)raw / 1.2f;

    return ESP_OK;
}

float bh1750_get_lux(bh1750_t *bh1750)
{
    return bh1750->lux;
}

static void bh1750_task(void *arg)
{
    i2c_drv_t *i2c = (i2c_drv_t *)arg;

    bh1750_t bh1750;

    esp_err_t err = bh1750_init(&bh1750,
                                i2c);

    if (err != ESP_OK)
    {
        ESP_LOGE(BH1750_TAG,
                 "bh1750_init failed: %s",
                 esp_err_to_name(err));

        vTaskDelete(NULL);
        return;
    }

    while (1)
    {
        err = bh1750_read(&bh1750);

        if (err == ESP_OK)
        {
            ESP_LOGI(BH1750_TAG,
                     "Light: %.2f lux",
                     bh1750_get_lux(&bh1750));
        }
        else
        {
            ESP_LOGW(BH1750_TAG,
                     "Read failed: %s",
                     esp_err_to_name(err));
        }

        vTaskDelay(pdMS_TO_TICKS(BH1750_READ_PERIOD_MS));
    }
}
esp_err_t bh1750_start(i2c_drv_t *i2c)
{
    ESP_RETURN_ON_FALSE(i2c != NULL,
                        ESP_ERR_INVALID_ARG,
                        BH1750_TAG,
                        "I2C Handle NULL");

    BaseType_t ok = xTaskCreate(
        bh1750_task,
        "bh1750_task",
        4096,
        i2c,
        5,
        NULL);

    return (ok == pdPASS) ? ESP_OK : ESP_ERR_NO_MEM;
}