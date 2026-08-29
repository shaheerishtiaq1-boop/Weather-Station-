#include "DHT22.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_rom_sys.h"
#include "esp_check.h"
#include "esp_log.h"

#define DHT22_TAG "DHT22"

#define DHT22_START_SIGNAL_MS      2
#define DHT22_RESPONSE_TIMEOUT_US  100
#define DHT22_BIT_TIMEOUT_US       100
#define DHT22_HIGH_TIME_THRESHOLD  40

static esp_err_t dht22_start_signal(dht22_t *dht);
static esp_err_t dht22_check_response(dht22_t *dht);
static uint8_t dht22_read_bit(dht22_t *dht);
static uint8_t dht22_read_byte(dht22_t *dht);

esp_err_t dht22_init(dht22_t *dht,
                     gpio_num_t gpio)
{
    ESP_RETURN_ON_FALSE(dht != NULL,
                        ESP_ERR_INVALID_ARG,
                        DHT22_TAG,
                        "Handle NULL");

    dht->temperature = 0.0f;
    dht->humidity = 0.0f;

    return one_wire_init(&dht->oneWire,
                         gpio);
}

static esp_err_t dht22_start_signal(dht22_t *dht)
{
    one_wire_set_output(&dht->oneWire);

    one_wire_write(&dht->oneWire,0);

    /* NOTE: vTaskDelay(pdMS_TO_TICKS(2)) previously used here rounds down to
     * 0 ticks at the default 100Hz FreeRTOS tick rate, so the line was
     * barely held low at all. DHT22 requires >=1ms low; busy-wait precisely
     * instead of relying on tick-granular scheduling delay. */
    one_wire_delay_us(DHT22_START_SIGNAL_MS * 1000);

    one_wire_write(&dht->oneWire,1);

    one_wire_delay_us(30);

    one_wire_set_input(&dht->oneWire);

    return ESP_OK;
}

static esp_err_t dht22_check_response(dht22_t *dht)
{
    ESP_RETURN_ON_ERROR(
        one_wire_wait_for_level(
            &dht->oneWire,
            0,
            DHT22_RESPONSE_TIMEOUT_US),
        DHT22_TAG,
        "No LOW response");

    ESP_RETURN_ON_ERROR(
        one_wire_wait_for_level(
            &dht->oneWire,
            1,
            DHT22_RESPONSE_TIMEOUT_US),
        DHT22_TAG,
        "No HIGH response");

    ESP_RETURN_ON_ERROR(
        one_wire_wait_for_level(
            &dht->oneWire,
            0,
            DHT22_RESPONSE_TIMEOUT_US),
        DHT22_TAG,
        "Sensor timeout");

    return ESP_OK;
}

/**
 * @brief Read a single bit. Called when the line has just gone low to
 *        signal the start of a new bit (per DHT22 timing). Waits for the
 *        line to rise, then measures how long it stays high: a short
 *        pulse (~26-28us) is a 0, a long pulse (~70us) is a 1.
 */
static uint8_t dht22_read_bit(dht22_t *dht)
{
    if (one_wire_wait_for_level(&dht->oneWire, 1, DHT22_BIT_TIMEOUT_US) != ESP_OK) {
        ESP_LOGW(DHT22_TAG, "bit timeout waiting for HIGH");
        return 0;
    }

    uint32_t high_duration_us = 0;
    if (one_wire_wait_for_level_timed(&dht->oneWire, 0, DHT22_BIT_TIMEOUT_US,
                                       &high_duration_us) != ESP_OK) {
        ESP_LOGW(DHT22_TAG, "bit timeout waiting for LOW");
        return 0;
    }

    return (high_duration_us > DHT22_HIGH_TIME_THRESHOLD) ? 1 : 0;
}

static uint8_t dht22_read_byte(dht22_t *dht)
{
    uint8_t value = 0;

    for (int i = 0; i < 8; i++) {
        value <<= 1;
        value |= dht22_read_bit(dht);
    }

    return value;
}

esp_err_t dht22_read(dht22_t *dht)
{
    ESP_RETURN_ON_FALSE(dht != NULL,
                        ESP_ERR_INVALID_ARG,
                        DHT22_TAG,
                        "Handle NULL");

    ESP_RETURN_ON_ERROR(dht22_start_signal(dht), DHT22_TAG, "start signal failed");
    ESP_RETURN_ON_ERROR(dht22_check_response(dht), DHT22_TAG, "no sensor response");

    uint8_t data[5] = {0};
    for (int i = 0; i < 5; i++) {
        data[i] = dht22_read_byte(dht);
    }

    /* Release the line back to idle-high */
    one_wire_set_input(&dht->oneWire);

    uint8_t checksum = (uint8_t)(data[0] + data[1] + data[2] + data[3]);
    if (checksum != data[4]) {
        ESP_LOGE(DHT22_TAG, "checksum mismatch: calc=0x%02X recv=0x%02X",
                 checksum, data[4]);
        return ESP_ERR_INVALID_CRC;
    }

    dht->humidity = ((data[0] << 8) | data[1]) / 10.0f;

    int16_t temp_raw = ((data[2] & 0x7F) << 8) | data[3];
    dht->temperature = temp_raw / 10.0f;
    if (data[2] & 0x80) {
        dht->temperature = -dht->temperature;
    }

    return ESP_OK;
}

float dht22_get_temperature(dht22_t *dht)
{
    return dht->temperature;
}

float dht22_get_humidity(dht22_t *dht)
{
    return dht->humidity;
}

/**
 * @brief Background task: owns its own dht22_t instance, reads every
 *        ~2.5s (the minimum DHT22 recovery interval), and logs results.
 */
static void dht22_task(void *arg)
{
    gpio_num_t gpio = (gpio_num_t)(intptr_t)arg;
    dht22_t dht;

    esp_err_t err = dht22_init(&dht, gpio);
    if (err != ESP_OK) {
        ESP_LOGE(DHT22_TAG, "dht22_init failed: %s", esp_err_to_name(err));
        vTaskDelete(NULL);
        return;
    }

    while (1) {
        err = dht22_read(&dht);
        if (err == ESP_OK) {
            ESP_LOGI(DHT22_TAG, "Temp: %.1f C  Humidity: %.1f %%",
                     dht22_get_temperature(&dht),
                     dht22_get_humidity(&dht));
        } else {
            ESP_LOGW(DHT22_TAG, "read failed: %s", esp_err_to_name(err));
        }

        vTaskDelay(pdMS_TO_TICKS(2500));
    }
}

esp_err_t dht22_start(gpio_num_t gpio)
{
    BaseType_t ok = xTaskCreate(dht22_task,
                                "dht22_task",
                                4096,
                                (void *)(intptr_t)gpio,
                                5,
                                NULL);

    return (ok == pdPASS) ? ESP_OK : ESP_ERR_NO_MEM;
}