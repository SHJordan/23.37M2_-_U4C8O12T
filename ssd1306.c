#include "ssd1306.h"
#include <stdlib.h>
#include <string.h>

#define SSD1306_SETCONTRAST 0x81
#define SSD1306_DISPLAYALLON_RESUME 0xA4
#define SSD1306_DISPLAYALLON 0xA5
#define SSD1306_NORMALDISPLAY 0xA6
#define SSD1306_INVERTDISPLAY 0xA7
#define SSD1306_DISPLAYOFF 0xAE
#define SSD1306_DISPLAYON 0xAF
#define SSD1306_SETDISPLAYOFFSET 0xD3
#define SSD1306_SETCOMPINS 0xDA
#define SSD1306_SETVCOMDETECT 0xDB
#define SSD1306_SETDISPLAYCLOCKDIV 0xD5
#define SSD1306_SETPRECHARGE 0xD9
#define SSD1306_SETMULTIPLEX 0xA8
#define SSD1306_SETLOWCOLUMN 0x00
#define SSD1306_SETHIGHCOLUMN 0x10
#define SSD1306_SETSTARTLINE 0x40
#define SSD1306_MEMORYMODE 0x20
#define SSD1306_COLUMNADDR 0x21
#define SSD1306_PAGEADDR 0x22
#define SSD1306_COMSCANINC 0xC0
#define SSD1306_COMSCANDEC 0xC8
#define SSD1306_SEGREMAP 0xA0
#define SSD1306_CHARGEPUMP 0x8D

static void write_cmd(ssd1306_t *display, uint8_t cmd)
{
    uint8_t buf[2] = {0x00, cmd};
    i2c_write_blocking(display->i2c_inst, SSD1306_ADDR, buf, 2, false);
}

static void write_data(ssd1306_t *display, uint8_t *buf, uint16_t len)
{
    uint8_t *temp = malloc(len + 1);
    temp[0] = 0x40;
    memcpy(temp + 1, buf, len);
    i2c_write_blocking(display->i2c_inst, SSD1306_ADDR, temp, len + 1, false);
    free(temp);
}

void ssd1306_init(ssd1306_t *display, uint16_t width, uint16_t height, i2c_inst_t *i2c_inst, uint8_t sda_pin)
{
    display->width = width;
    display->height = height;
    display->i2c_inst = i2c_inst;
    display->buffer = malloc((width * height) / 8);
    memset(display->buffer, 0, (width * height) / 8);

    write_cmd(display, SSD1306_DISPLAYOFF);
    write_cmd(display, SSD1306_SETDISPLAYCLOCKDIV);
    write_cmd(display, 0x80);
    write_cmd(display, SSD1306_SETMULTIPLEX);
    write_cmd(display, height - 1);
    write_cmd(display, SSD1306_SETDISPLAYOFFSET);
    write_cmd(display, 0x00);
    write_cmd(display, SSD1306_SETSTARTLINE | 0x00);
    write_cmd(display, SSD1306_CHARGEPUMP);
    write_cmd(display, 0x14);
    write_cmd(display, SSD1306_MEMORYMODE);
    write_cmd(display, 0x00);
    write_cmd(display, SSD1306_SEGREMAP | 0x1);
    write_cmd(display, SSD1306_COMSCANDEC);
    write_cmd(display, SSD1306_SETCOMPINS);
    write_cmd(display, height == 64 ? 0x12 : 0x02);
    write_cmd(display, SSD1306_SETCONTRAST);
    write_cmd(display, height == 64 ? 0xCF : 0x8F);
    write_cmd(display, SSD1306_SETPRECHARGE);
    write_cmd(display, 0xF1);
    write_cmd(display, SSD1306_SETVCOMDETECT);
    write_cmd(display, 0x40);
    write_cmd(display, SSD1306_DISPLAYALLON_RESUME);
    write_cmd(display, SSD1306_NORMALDISPLAY);
    write_cmd(display, SSD1306_DISPLAYON);
}

void ssd1306_clear(ssd1306_t *display)
{
    memset(display->buffer, 0, (display->width * display->height) / 8);
}

void ssd1306_show(ssd1306_t *display)
{
    write_cmd(display, SSD1306_COLUMNADDR);
    write_cmd(display, 0);
    write_cmd(display, display->width - 1);
    write_cmd(display, SSD1306_PAGEADDR);
    write_cmd(display, 0);
    write_cmd(display, display->height / 8 - 1);
    write_data(display, display->buffer, (display->width * display->height) / 8);
}

void ssd1306_draw_pixel(ssd1306_t *display, uint16_t x, uint16_t y, bool color)
{
    if (x >= display->width || y >= display->height)
        return;

    uint16_t index = x + (y / 8) * display->width;
    uint8_t bit = y % 8;

    if (color)
        display->buffer[index] |= (1 << bit);
    else
        display->buffer[index] &= ~(1 << bit);
}

void ssd1306_draw_line(ssd1306_t *display, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = (dx > dy ? dx : -dy) / 2;

    while (1)
    {
        ssd1306_draw_pixel(display, x0, y0, true);
        if (x0 == x1 && y0 == y1)
            break;
        int e2 = err;
        if (e2 > -dx)
        {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dy)
        {
            err += dx;
            y0 += sy;
        }
    }
}

void ssd1306_draw_rectangle(ssd1306_t *display, uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    ssd1306_draw_line(display, x, y, x + width, y);
    ssd1306_draw_line(display, x + width, y, x + width, y + height);
    ssd1306_draw_line(display, x + width, y + height, x, y + height);
    ssd1306_draw_line(display, x, y + height, x, y);
}

void ssd1306_draw_square(ssd1306_t *display, uint16_t x, uint16_t y, uint16_t size, bool fill)
{
    if (fill)
    {
        for (uint16_t i = 0; i < size; i++)
        {
            for (uint16_t j = 0; j < size; j++)
            {
                ssd1306_draw_pixel(display, x + i, y + j, true);
            }
        }
    }
    else
    {
        ssd1306_draw_rectangle(display, x, y, size - 1, size - 1);
    }
}