/**
 * @file ssd1306.h
 * @brief SSD1306 128x64 OLED display driver, built on the generic i2cdrv.
 *
 * Maintains an in-memory framebuffer; call ssd1306_display() to push it to
 * the physical screen. Text functions are page-aligned (y must be a
 * multiple of 8) to keep glyph rendering simple and fast.
 */

#ifndef SSD1306_H
#define SSD1306_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "i2cdrv.h"

#define SSD1306_WIDTH   128
#define SSD1306_HEIGHT  64
#define SSD1306_PAGES   (SSD1306_HEIGHT / 8)

/**
 * @brief SSD1306 display handle.
 */
typedef struct
{
    i2c_device_t device;

    uint8_t buffer[SSD1306_PAGES][SSD1306_WIDTH];

    uint8_t cursorX;

    uint8_t cursorY;

} ssd1306_t;

/**
 * @brief Initialize the SSD1306 display.
 *
 * Registers the display on an already-initialized I2C bus and sends the
 * standard SSD1306 128x64 init command sequence. Clears the framebuffer
 * (does not push it - call ssd1306_display() afterward).
 *
 * @param oled     Pointer to display handle.
 * @param i2c      Pointer to initialized I2C bus.
 * @param address  7-bit I2C address (typically 0x3C, sometimes 0x3D).
 *
 * @return ESP_OK on success.
 */
esp_err_t ssd1306_init(ssd1306_t *oled,
                       i2c_drv_t *i2c,
                       uint16_t address);

/**
 * @brief Clear the in-memory framebuffer (does not touch the screen until
 *        ssd1306_display() is called).
 */
esp_err_t ssd1306_clear(ssd1306_t *oled);

/**
 * @brief Push the in-memory framebuffer to the physical screen.
 */
esp_err_t ssd1306_display(ssd1306_t *oled);

/**
 * @brief Set (or clear) a single pixel in the framebuffer.
 *
 * @param oled   Display handle.
 * @param x      Column, 0..SSD1306_WIDTH-1.
 * @param y      Row, 0..SSD1306_HEIGHT-1.
 * @param color  true = pixel on, false = pixel off.
 */
void ssd1306_set_pixel(ssd1306_t *oled,
                       uint8_t x,
                       uint8_t y,
                       bool color);

/**
 * @brief Set the text cursor position for subsequent ssd1306_print().
 *
 * @param oled  Display handle.
 * @param x     Column in pixels, 0..SSD1306_WIDTH-1.
 * @param y     Row - MUST be a multiple of 8 (page-aligned), 0..56.
 */
void ssd1306_set_cursor(ssd1306_t *oled,
                        uint8_t x,
                        uint8_t y);

/**
 * @brief Draw a single character at the given position (page-aligned y).
 *
 * @param oled  Display handle.
 * @param x     Column in pixels.
 * @param y     Row - MUST be a multiple of 8.
 * @param c     Character to draw (printable ASCII).
 */
void ssd1306_draw_char(ssd1306_t *oled,
                       uint8_t x,
                       uint8_t y,
                       char c);

/**
 * @brief Draw a null-terminated string starting at the current cursor
 *        position (set via ssd1306_set_cursor()), advancing the cursor as
 *        it goes. Does not wrap - characters past the right edge are
 *        clipped.
 */
void ssd1306_print(ssd1306_t *oled, const char *str);

/**
 * @brief Draw a single character at a larger size (page-aligned y).
 *
 * Each font pixel becomes a scale x scale block, so a scale of 2 makes
 * text roughly double-height/double-width, scale of 3 triple, etc.
 *
 * @param oled   Display handle.
 * @param x      Column in pixels.
 * @param y      Row - MUST be a multiple of 8.
 * @param c      Character to draw (printable ASCII).
 * @param scale  Size multiplier (1 = normal size, 2 = double, etc).
 */
void ssd1306_draw_char_scaled(ssd1306_t *oled,
                              uint8_t x,
                              uint8_t y,
                              char c,
                              uint8_t scale);

/**
 * @brief Draw a null-terminated string at a larger size, starting at the
 *        current cursor position, advancing the cursor as it goes. Does
 *        not wrap - characters past the right edge are clipped.
 *
 * @param oled   Display handle.
 * @param str    String to draw.
 * @param scale  Size multiplier (1 = normal size, 2 = double, etc).
 */
void ssd1306_print_scaled(ssd1306_t *oled, const char *str, uint8_t scale);

#ifdef __cplusplus
}
#endif

#endif /* SSD1306_H */