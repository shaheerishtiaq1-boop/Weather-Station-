/**
 * @file ssd1306.c
 * @brief SSD1306 128x64 OLED display driver.
 */

#include "ssd1306.h"

#include "esp_check.h"
#include "esp_log.h"

/**
 * Internal 5x7 pixel font table, ASCII 0x20-0x7E.
 *
 * Each glyph is 5 columns x 7 rows; each byte represents one column,
 * bit 0 = top row, bit 6 = bottom row (bit 7 unused).
 */

static const uint8_t ssd1306_font5x7[95][5] = {
    {0x00,0x00,0x00,0x00,0x00}, /* ' ' 0x20 */
    {0x00,0x00,0x5F,0x00,0x00}, /* '!' */
    {0x00,0x07,0x00,0x07,0x00}, /* '"' */
    {0x14,0x7F,0x14,0x7F,0x14}, /* '#' */
    {0x24,0x2A,0x7F,0x2A,0x12}, /* '$' */
    {0x23,0x13,0x08,0x64,0x62}, /* '%' */
    {0x36,0x49,0x56,0x20,0x50}, /* '&' */
    {0x00,0x08,0x07,0x03,0x00}, /* ''' */
    {0x00,0x1C,0x22,0x41,0x00}, /* '(' */
    {0x00,0x41,0x22,0x1C,0x00}, /* ')' */
    {0x2A,0x1C,0x7F,0x1C,0x2A}, /* '*' */
    {0x08,0x08,0x3E,0x08,0x08}, /* '+' */
    {0x00,0x80,0x70,0x30,0x00}, /* ',' */
    {0x08,0x08,0x08,0x08,0x08}, /* '-' */
    {0x00,0x00,0x60,0x60,0x00}, /* '.' */
    {0x20,0x10,0x08,0x04,0x02}, /* '/' */
    {0x3E,0x51,0x49,0x45,0x3E}, /* '0' */
    {0x00,0x42,0x7F,0x40,0x00}, /* '1' */
    {0x42,0x61,0x51,0x49,0x46}, /* '2' */
    {0x21,0x41,0x45,0x4B,0x31}, /* '3' */
    {0x18,0x14,0x12,0x7F,0x10}, /* '4' */
    {0x27,0x45,0x45,0x45,0x39}, /* '5' */
    {0x3C,0x4A,0x49,0x49,0x30}, /* '6' */
    {0x01,0x71,0x09,0x05,0x03}, /* '7' */
    {0x36,0x49,0x49,0x49,0x36}, /* '8' */
    {0x06,0x49,0x49,0x29,0x1E}, /* '9' */
    {0x00,0x00,0x36,0x36,0x00}, /* ':' */
    {0x00,0x80,0x76,0x36,0x00}, /* ';' */
    {0x08,0x14,0x22,0x41,0x00}, /* '<' */
    {0x14,0x14,0x14,0x14,0x14}, /* '=' */
    {0x00,0x41,0x22,0x14,0x08}, /* '>' */
    {0x02,0x01,0x51,0x09,0x06}, /* '?' */
    {0x32,0x49,0x79,0x41,0x3E}, /* '@' */
    {0x7E,0x11,0x11,0x11,0x7E}, /* 'A' */
    {0x7F,0x49,0x49,0x49,0x36}, /* 'B' */
    {0x3E,0x41,0x41,0x41,0x22}, /* 'C' */
    {0x7F,0x41,0x41,0x22,0x1C}, /* 'D' */
    {0x7F,0x49,0x49,0x49,0x41}, /* 'E' */
    {0x7F,0x09,0x09,0x09,0x01}, /* 'F' */
    {0x3E,0x41,0x49,0x49,0x7A}, /* 'G' */
    {0x7F,0x08,0x08,0x08,0x7F}, /* 'H' */
    {0x00,0x41,0x7F,0x41,0x00}, /* 'I' */
    {0x20,0x40,0x41,0x3F,0x01}, /* 'J' */
    {0x7F,0x08,0x14,0x22,0x41}, /* 'K' */
    {0x7F,0x40,0x40,0x40,0x40}, /* 'L' */
    {0x7F,0x02,0x0C,0x02,0x7F}, /* 'M' */
    {0x7F,0x04,0x08,0x10,0x7F}, /* 'N' */
    {0x3E,0x41,0x41,0x41,0x3E}, /* 'O' */
    {0x7F,0x09,0x09,0x09,0x06}, /* 'P' */
    {0x3E,0x41,0x51,0x21,0x5E}, /* 'Q' */
    {0x7F,0x09,0x19,0x29,0x46}, /* 'R' */
    {0x46,0x49,0x49,0x49,0x31}, /* 'S' */
    {0x01,0x01,0x7F,0x01,0x01}, /* 'T' */
    {0x3F,0x40,0x40,0x40,0x3F}, /* 'U' */
    {0x1F,0x20,0x40,0x20,0x1F}, /* 'V' */
    {0x3F,0x40,0x38,0x40,0x3F}, /* 'W' */
    {0x63,0x14,0x08,0x14,0x63}, /* 'X' */
    {0x07,0x08,0x70,0x08,0x07}, /* 'Y' */
    {0x61,0x51,0x49,0x45,0x43}, /* 'Z' */
    {0x00,0x7F,0x41,0x41,0x00}, /* '[' */
    {0x02,0x04,0x08,0x10,0x20}, /* '\' */
    {0x00,0x41,0x41,0x7F,0x00}, /* ']' */
    {0x04,0x02,0x01,0x02,0x04}, /* '^' */
    {0x40,0x40,0x40,0x40,0x40}, /* '_' */
    {0x00,0x01,0x02,0x04,0x00}, /* '`' */
    {0x20,0x54,0x54,0x54,0x78}, /* 'a' */
    {0x7F,0x48,0x44,0x44,0x38}, /* 'b' */
    {0x38,0x44,0x44,0x44,0x20}, /* 'c' */
    {0x38,0x44,0x44,0x48,0x7F}, /* 'd' */
    {0x38,0x54,0x54,0x54,0x18}, /* 'e' */
    {0x08,0x7E,0x09,0x01,0x02}, /* 'f' */
    {0x0C,0x52,0x52,0x52,0x3E}, /* 'g' */
    {0x7F,0x08,0x04,0x04,0x78}, /* 'h' */
    {0x00,0x44,0x7D,0x40,0x00}, /* 'i' */
    {0x20,0x40,0x44,0x3D,0x00}, /* 'j' */
    {0x7F,0x10,0x28,0x44,0x00}, /* 'k' */
    {0x00,0x41,0x7F,0x40,0x00}, /* 'l' */
    {0x7C,0x04,0x18,0x04,0x78}, /* 'm' */
    {0x7C,0x08,0x04,0x04,0x78}, /* 'n' */
    {0x38,0x44,0x44,0x44,0x38}, /* 'o' */
    {0x7C,0x14,0x14,0x14,0x08}, /* 'p' */
    {0x08,0x14,0x14,0x18,0x7C}, /* 'q' */
    {0x7C,0x08,0x04,0x04,0x08}, /* 'r' */
    {0x48,0x54,0x54,0x54,0x20}, /* 's' */
    {0x04,0x3F,0x44,0x40,0x20}, /* 't' */
    {0x3C,0x40,0x40,0x20,0x7C}, /* 'u' */
    {0x1C,0x20,0x40,0x20,0x1C}, /* 'v' */
    {0x3C,0x40,0x30,0x40,0x3C}, /* 'w' */
    {0x44,0x28,0x10,0x28,0x44}, /* 'x' */
    {0x0C,0x50,0x50,0x50,0x3C}, /* 'y' */
    {0x44,0x64,0x54,0x4C,0x44}, /* 'z' */
    {0x00,0x08,0x36,0x41,0x00}, /* '{' */
    {0x00,0x00,0x7F,0x00,0x00}, /* '|' */
    {0x00,0x41,0x36,0x08,0x00}, /* '}' */
    {0x08,0x04,0x08,0x10,0x08}, /* '~' */
};


#define SSD1306_TAG "SSD1306"

#define SSD1306_CTRL_CMD   0x00
#define SSD1306_CTRL_DATA  0x40

static esp_err_t ssd1306_write_cmds(ssd1306_t *oled, const uint8_t *cmds, size_t len);
static esp_err_t ssd1306_write_page(ssd1306_t *oled, uint8_t page);

esp_err_t ssd1306_init(ssd1306_t *oled,
                       i2c_drv_t *i2c,
                       uint16_t address)
{
    ESP_RETURN_ON_FALSE(oled != NULL,
                        ESP_ERR_INVALID_ARG,
                        SSD1306_TAG,
                        "Handle NULL");

    ESP_RETURN_ON_FALSE(i2c != NULL,
                        ESP_ERR_INVALID_ARG,
                        SSD1306_TAG,
                        "I2C Handle NULL");

    oled->cursorX = 0;
    oled->cursorY = 0;

    ESP_RETURN_ON_ERROR(
        i2c_drv_add_device(i2c, &oled->device, address),
        SSD1306_TAG,
        "Failed to add device");

    static const uint8_t initCmds[] = {
        0xAE,             /* display off */
        0x20, 0x02,       /* memory addressing mode: page addressing */
        0xB0,             /* page start address 0 */
        0xC8,             /* COM output scan direction, remapped */
        0x00,             /* lower column address = 0 */
        0x10,             /* higher column address = 0 */
        0x40,             /* display start line = 0 */
        0x81, 0x7F,       /* contrast control */
        0xA1,             /* segment re-map */
        0xA6,             /* normal display (not inverted) */
        0xA8, 0x3F,       /* multiplex ratio = 64 (for 128x64) */
        0xA4,             /* entire display follows RAM content */
        0xD3, 0x00,       /* display offset = 0 */
        0xD5, 0x80,       /* display clock divide ratio / osc freq */
        0xD9, 0xF1,       /* pre-charge period */
        0xDA, 0x12,       /* COM pins hardware config (128x64) */
        0xDB, 0x40,       /* VCOMH deselect level */
        0x8D, 0x14,       /* charge pump enable */
        0xAF,             /* display on */
    };

    ESP_RETURN_ON_ERROR(
        ssd1306_write_cmds(oled, initCmds, sizeof(initCmds)),
        SSD1306_TAG,
        "Init sequence failed");

    return ssd1306_clear(oled);
}

esp_err_t ssd1306_clear(ssd1306_t *oled)
{
    ESP_RETURN_ON_FALSE(oled != NULL,
                        ESP_ERR_INVALID_ARG,
                        SSD1306_TAG,
                        "Handle NULL");

    for (int page = 0; page < SSD1306_PAGES; page++) {
        for (int col = 0; col < SSD1306_WIDTH; col++) {
            oled->buffer[page][col] = 0x00;
        }
    }

    oled->cursorX = 0;
    oled->cursorY = 0;

    return ESP_OK;
}

esp_err_t ssd1306_display(ssd1306_t *oled)
{
    ESP_RETURN_ON_FALSE(oled != NULL,
                        ESP_ERR_INVALID_ARG,
                        SSD1306_TAG,
                        "Handle NULL");

    for (uint8_t page = 0; page < SSD1306_PAGES; page++) {
        ESP_RETURN_ON_ERROR(
            ssd1306_write_page(oled, page),
            SSD1306_TAG,
            "Page write failed");
    }

    return ESP_OK;
}

void ssd1306_set_pixel(ssd1306_t *oled,
                       uint8_t x,
                       uint8_t y,
                       bool color)
{
    if (oled == NULL || x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) {
        return;
    }

    uint8_t page = y / 8;
    uint8_t bit  = y % 8;

    if (color) {
        oled->buffer[page][x] |= (1 << bit);
    } else {
        oled->buffer[page][x] &= ~(1 << bit);
    }
}

void ssd1306_set_cursor(ssd1306_t *oled,
                        uint8_t x,
                        uint8_t y)
{
    if (oled == NULL) {
        return;
    }

    oled->cursorX = x;
    oled->cursorY = y;
}

void ssd1306_draw_char(ssd1306_t *oled,
                       uint8_t x,
                       uint8_t y,
                       char c)
{
    if (oled == NULL) {
        return;
    }

    if (c < 0x20 || c > 0x7E) {
        c = ' ';
    }

    if (x > (SSD1306_WIDTH - 6) || y >= SSD1306_HEIGHT) {
        return;
    }

    uint8_t page = y / 8;
    const uint8_t *glyph = ssd1306_font5x7[(uint8_t)c - 0x20];

    for (int col = 0; col < 5; col++) {
        oled->buffer[page][x + col] = glyph[col];
    }
    oled->buffer[page][x + 5] = 0x00; /* 1px spacing column */
}

void ssd1306_print(ssd1306_t *oled, const char *str)
{
    if (oled == NULL || str == NULL) {
        return;
    }

    while (*str != '\0') {
        if (oled->cursorX > (SSD1306_WIDTH - 6)) {
            break; /* out of room on this line - no wrap */
        }

        ssd1306_draw_char(oled, oled->cursorX, oled->cursorY, *str);
        oled->cursorX += 6;
        str++;
    }
}

void ssd1306_draw_char_scaled(ssd1306_t *oled,
                              uint8_t x,
                              uint8_t y,
                              char c,
                              uint8_t scale)
{
    if (oled == NULL) {
        return;
    }

    if (c < 0x20 || c > 0x7E) {
        c = ' ';
    }

    if (scale == 0) {
        scale = 1;
    }

    const uint8_t *glyph = ssd1306_font5x7[(uint8_t)c - 0x20];

    /* Glyph body: 5 columns x 7 rows, each font pixel expanded to a
     * scale x scale block. Uses set_pixel per-bit (not a raw byte copy),
     * so this also works for non-page-aligned y if you use it that way. */
    for (int col = 0; col < 5; col++) {
        uint8_t line = glyph[col];
        for (int row = 0; row < 7; row++) {
            bool on = (line >> row) & 0x01;
            for (int sx = 0; sx < scale; sx++) {
                for (int sy = 0; sy < scale; sy++) {
                    ssd1306_set_pixel(oled,
                                      (uint8_t)(x + col * scale + sx),
                                      (uint8_t)(y + row * scale + sy),
                                      on);
                }
            }
        }
    }

    /* 1-column-wide (scaled) spacing gap after the glyph, cleared so old
     * pixels underneath don't linger if re-drawing over previous text. */
    for (int sy = 0; sy < 7 * scale; sy++) {
        for (int sx = 0; sx < scale; sx++) {
            ssd1306_set_pixel(oled,
                              (uint8_t)(x + 5 * scale + sx),
                              (uint8_t)(y + sy),
                              false);
        }
    }
}

void ssd1306_print_scaled(ssd1306_t *oled, const char *str, uint8_t scale)
{
    if (oled == NULL || str == NULL) {
        return;
    }

    if (scale == 0) {
        scale = 1;
    }

    uint8_t charWidth = (uint8_t)(6 * scale);

    while (*str != '\0') {
        if ((oled->cursorX + charWidth) > SSD1306_WIDTH) {
            break; /* out of room on this line - no wrap */
        }

        ssd1306_draw_char_scaled(oled, oled->cursorX, oled->cursorY, *str, scale);
        oled->cursorX += charWidth;
        str++;
    }
}

/**
 * @brief Send a sequence of command bytes (control byte 0x00 prefix).
 */
static esp_err_t ssd1306_write_cmds(ssd1306_t *oled, const uint8_t *cmds, size_t len)
{
    uint8_t txBuffer[32]; /* init sequence is well under 32 bytes */

    if (len + 1 > sizeof(txBuffer)) {
        return ESP_ERR_INVALID_SIZE;
    }

    txBuffer[0] = SSD1306_CTRL_CMD;
    for (size_t i = 0; i < len; i++) {
        txBuffer[1 + i] = cmds[i];
    }

    return i2c_drv_write(&oled->device, txBuffer, len + 1);
}

/**
 * @brief Push one page (128 bytes) of the framebuffer to the display.
 */
static esp_err_t ssd1306_write_page(ssd1306_t *oled, uint8_t page)
{
    uint8_t setPageCmds[] = {
        (uint8_t)(0xB0 + page), /* set page start address */
        0x00,                    /* lower column address = 0 */
        0x10,                    /* higher column address = 0 */
    };

    ESP_RETURN_ON_ERROR(
        ssd1306_write_cmds(oled, setPageCmds, sizeof(setPageCmds)),
        SSD1306_TAG,
        "Set page address failed");

    uint8_t txBuffer[SSD1306_WIDTH + 1];
    txBuffer[0] = SSD1306_CTRL_DATA;
    for (int col = 0; col < SSD1306_WIDTH; col++) {
        txBuffer[1 + col] = oled->buffer[page][col];
    }

    return i2c_drv_write(&oled->device, txBuffer, sizeof(txBuffer));
}