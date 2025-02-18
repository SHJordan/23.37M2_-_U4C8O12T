// main.c
#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include "ssd1306.h"
#include <string.h>

// Pin definitions
#define LED_RED 11
#define LED_GREEN 12
#define LED_BLUE 13
#define BTN_JOY 22
#define BTN_A 5
#define JOY_X 26
#define JOY_Y 27
#define I2C_SDA 14
#define I2C_SCL 15
#define I2C_PORT i2c0

// Display dimensions
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64
#define SQUARE_SIZE 8

// Constants
#define ADC_MAX 4095
#define PWM_MAX 65535
#define DEBOUNCE_MS 200

// Global variables
volatile bool green_led_state = false;
volatile uint8_t border_style = 0;
volatile bool pwm_enabled = true;
volatile uint64_t last_btn_joy_time = 0;
volatile uint64_t last_btn_a_time = 0;

// Function prototypes
void init_gpios(void);
void init_pwm(void);
void init_adc(void);
void init_i2c(void);
void update_display(ssd1306_t *disp, uint16_t x, uint16_t y);
void gpio_callback(uint gpio, uint32_t events);
uint16_t map_value(uint16_t value, uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max);

int main()
{
    stdio_init_all();

    // Initialize peripherals
    init_gpios();
    init_pwm();
    init_adc();
    init_i2c();

    // Initialize display
    ssd1306_t display;
    ssd1306_init(&display, DISPLAY_WIDTH, DISPLAY_HEIGHT, I2C_PORT, I2C_SDA);

    while (1)
    {
        // Read joystick values
        adc_select_input(0);
        uint16_t x_val = adc_read();
        adc_select_input(1);
        uint16_t y_val = adc_read();

        // Update LED intensities if enabled
        if (pwm_enabled)
        {
            // X axis controls red LED
            uint16_t red_intensity = abs(x_val - 2048) * 2;
            pwm_set_gpio_level(LED_RED, map_value(red_intensity, 0, ADC_MAX, 0, PWM_MAX));

            // Y axis controls blue LED
            uint16_t blue_intensity = abs(y_val - 2048) * 2;
            pwm_set_gpio_level(LED_BLUE, map_value(blue_intensity, 0, ADC_MAX, 0, PWM_MAX));
        }
        else
        {
            pwm_set_gpio_level(LED_RED, 0);
            pwm_set_gpio_level(LED_BLUE, 0);
        }

        // Update display
        uint16_t square_x = map_value(x_val, 0, ADC_MAX, 0, DISPLAY_WIDTH - SQUARE_SIZE);
        uint16_t square_y = map_value(y_val, 0, ADC_MAX, 0, DISPLAY_HEIGHT - SQUARE_SIZE);
        update_display(&display, square_x, square_y);

        sleep_ms(10);
    }
}

void init_gpios()
{
    // Initialize LED pins for PWM
    gpio_set_function(LED_RED, GPIO_FUNC_PWM);
    gpio_set_function(LED_GREEN, GPIO_FUNC_PWM);
    gpio_set_function(LED_BLUE, GPIO_FUNC_PWM);

    // Initialize button pins with pull-ups and interrupts
    gpio_init(BTN_JOY);
    gpio_set_dir(BTN_JOY, GPIO_IN);
    gpio_pull_up(BTN_JOY);
    gpio_set_irq_enabled_with_callback(BTN_JOY, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    gpio_init(BTN_A);
    gpio_set_dir(BTN_A, GPIO_IN);
    gpio_pull_up(BTN_A);
    gpio_set_irq_enabled_with_callback(BTN_A, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
}

void init_pwm()
{
    // Configure PWM for LEDs
    uint slice_red = pwm_gpio_to_slice_num(LED_RED);
    uint slice_green = pwm_gpio_to_slice_num(LED_GREEN);
    uint slice_blue = pwm_gpio_to_slice_num(LED_BLUE);

    pwm_set_wrap(slice_red, PWM_MAX);
    pwm_set_wrap(slice_green, PWM_MAX);
    pwm_set_wrap(slice_blue, PWM_MAX);

    pwm_set_enabled(slice_red, true);
    pwm_set_enabled(slice_green, true);
    pwm_set_enabled(slice_blue, true);
}

void init_adc()
{
    adc_init();
    adc_gpio_init(JOY_X);
    adc_gpio_init(JOY_Y);
}

void init_i2c()
{
    i2c_init(i2c0, 400000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
}

void update_display(ssd1306_t *disp, uint16_t x, uint16_t y)
{
    ssd1306_clear(disp);

    // Draw border based on current style
    switch (border_style)
    {
    case 0:
        // No border
        break;
    case 1:
        // Single line border
        ssd1306_draw_rectangle(disp, 0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1);
        break;
    case 2:
        // Double line border
        ssd1306_draw_rectangle(disp, 0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1);
        ssd1306_draw_rectangle(disp, 2, 2, DISPLAY_WIDTH - 3, DISPLAY_HEIGHT - 3);
        break;
    }

    // Draw moving square
    ssd1306_draw_square(disp, x, y, SQUARE_SIZE, true);

    ssd1306_show(disp);
}

void gpio_callback(uint gpio, uint32_t events)
{
    uint64_t current_time = time_us_64();

    if (gpio == BTN_JOY)
    {
        if (current_time - last_btn_joy_time > DEBOUNCE_MS * 1000)
        {
            green_led_state = !green_led_state;
            pwm_set_gpio_level(LED_GREEN, green_led_state ? PWM_MAX : 0);
            border_style = (border_style + 1) % 3;
            last_btn_joy_time = current_time;
        }
    }
    else if (gpio == BTN_A)
    {
        if (current_time - last_btn_a_time > DEBOUNCE_MS * 1000)
        {
            pwm_enabled = !pwm_enabled;
            last_btn_a_time = current_time;
        }
    }
}

uint16_t map_value(uint16_t value, uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max)
{
    return (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}