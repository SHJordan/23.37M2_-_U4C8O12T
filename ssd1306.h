// ssd1306.h
#ifndef SSD1306_H
#define SSD1306_H

#include "hardware/i2c.h"
#include <stdint.h>
#include <stdbool.h>

#define SSD1306_ADDR 0x3C

typedef struct
{
    uint16_t width;
    uint16_t height;
    i2c_inst_t *i2c_inst;
    uint8_t *buffer;
} ssd1306_t;

void ssd1306_init(ssd1306_t *display, uint16_t width, uint16_t height, i2c_inst_t *i2c_inst, uint8_t sda_pin);
void ssd1306_clear(ssd1306_t *display);
void ssd1306_show(ssd1306_t *display);
void ssd1306_draw_pixel(ssd1306_t *display, uint16_t x, uint16_t y, bool color);
void ssd1306_draw_line(ssd1306_t *display, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void ssd1306_draw_rectangle(ssd1306_t *display, uint16_t x, uint16_t y, uint16_t width, uint16_t height);
void ssd1306_draw_square(ssd1306_t *display, uint16_t x, uint16_t y, uint16_t size, bool fill);

#endif // SSD1306_H