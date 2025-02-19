#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include <string.h>

// Definições dos pinos
#define LED_RED 13
#define LED_GREEN 12
#define LED_BLUE 11
#define BTN_JOY 22
#define BTN_A 5
#define JOY_X 26
#define JOY_Y 27
#define I2C_SDA 14
#define I2C_SCL 15
#define I2C_PORT i2c1
#define I2C_ADDR 0x3C

// Dimensões do display
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64
#define SQUARE_SIZE 8

// Constantes
#define ADC_MAX 4095
#define PWM_MAX 65535
#define DEBOUNCE_MS 200

// Variáveis globais
volatile bool green_led_state = false;
volatile uint8_t border_style = 0;
volatile bool pwm_enabled = true;
volatile uint64_t last_btn_joy_time = 0;
volatile uint64_t last_btn_a_time = 0;

ssd1306_t display;
uint16_t square_x;
uint16_t square_y;

// Protótipos das funções
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

    // Inicializa periféricos
    init_gpios();
    init_pwm();
    init_adc();
    init_i2c();

    while (1)
    {
        // Lê valores do joystick
        adc_select_input(0);
        uint16_t x_val = adc_read();
        adc_select_input(1);
        uint16_t y_val = adc_read();

        // Atualiza intensidade dos LEDs se habilitado
        if (pwm_enabled)
        {
            // Eixo X controla LED vermelho
            uint16_t red_intensity = abs(x_val - 2048) * 2;
            pwm_set_gpio_level(LED_RED, map_value(red_intensity, 0, ADC_MAX, 0, PWM_MAX));
            printf("Red: %d\n", red_intensity);

            // Eixo Y controla LED azul
            uint16_t blue_intensity = abs(y_val - 2048) * 2;
            pwm_set_gpio_level(LED_BLUE, map_value(blue_intensity, 0, ADC_MAX, 0, PWM_MAX));
            printf("Blue: %d\n", blue_intensity);
        }
        else
        {
            pwm_set_gpio_level(LED_RED, 0);
            pwm_set_gpio_level(LED_BLUE, 0);
        }

        // Atualiza display
        square_x = map_value(x_val, 0, ADC_MAX, 0, DISPLAY_WIDTH - SQUARE_SIZE);
        square_y = map_value(y_val, 0, ADC_MAX, 0, DISPLAY_HEIGHT - SQUARE_SIZE);
        update_display(&display, square_x, square_y);

        sleep_ms(100);
    }
}

void init_gpios()
{
    // Inicializa pinos dos LEDs para PWM
    gpio_set_function(LED_RED, GPIO_FUNC_PWM);
    gpio_set_function(LED_BLUE, GPIO_FUNC_PWM);

    gpio_init(LED_GREEN);
    gpio_set_dir(LED_GREEN, GPIO_OUT);
    gpio_put(LED_GREEN, green_led_state);

    // Inicializa pinos dos botões com pull-ups e interrupções
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
    // Configura PWM para os LEDs
    uint slice_red = pwm_gpio_to_slice_num(LED_RED);
    uint slice_blue = pwm_gpio_to_slice_num(LED_BLUE);

    pwm_set_wrap(slice_red, PWM_MAX);
    pwm_set_wrap(slice_blue, PWM_MAX);

    pwm_set_enabled(slice_red, true);
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
    i2c_init(I2C_PORT, 400000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    ssd1306_init(&display, DISPLAY_WIDTH, DISPLAY_HEIGHT, false, I2C_ADDR, I2C_PORT);
    ssd1306_config(&display);
    ssd1306_fill(&display, false);
    ssd1306_send_data(&display);
}

void update_display(ssd1306_t *disp, uint16_t x, uint16_t y)
{
    ssd1306_fill(disp, false);

    // Desenha borda baseado no estilo atual
    switch (border_style)
    {
    case 0:
        // Sem borda
        ssd1306_line(disp, 0, DISPLAY_HEIGHT - 1, DISPLAY_WIDTH, DISPLAY_HEIGHT - 1, true);    // linha de baixo
        ssd1306_line(disp, 0, DISPLAY_HEIGHT - 1, 0, 0, true);                                 // linha da esquerda
        ssd1306_line(disp, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1, DISPLAY_WIDTH - 1, 0, true); // linha da direita
        ssd1306_line(disp, 0, 0, DISPLAY_WIDTH, 0, true);                                      // linha de cima
        break;
    case 1:
        // Borda com linha simples
        ssd1306_line(disp, 0, DISPLAY_HEIGHT - 1, DISPLAY_WIDTH, DISPLAY_HEIGHT - 1, true);    // linha de baixo
        ssd1306_line(disp, 0, DISPLAY_HEIGHT - 1, 0, 0, true);                                 // linha da esquerda
        ssd1306_line(disp, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1, DISPLAY_WIDTH - 1, 0, true); // linha da direita
        ssd1306_line(disp, 0, 0, DISPLAY_WIDTH, 0, true);                                      // linha de cima
        break;
    case 2:
        // Borda com linha dupla
        ssd1306_line(disp, 2, DISPLAY_HEIGHT - 3, DISPLAY_WIDTH - 2, DISPLAY_HEIGHT - 3, true);
        ssd1306_line(disp, 2, DISPLAY_HEIGHT - 3, 2, 2, true);
        ssd1306_line(disp, DISPLAY_WIDTH - 2, DISPLAY_HEIGHT - 3, DISPLAY_WIDTH - 2, 2, true);
        ssd1306_line(disp, 2, 2, DISPLAY_WIDTH - 2, 2, true);
        break;
    }

    // Desenha quadrado móvel
    // ssd1306_rect(disp, x, y, SQUARE_SIZE, SQUARE_SIZE, true, true);

    ssd1306_send_data(disp);
}

void gpio_callback(uint gpio, uint32_t events)
{
    uint64_t current_time = time_us_64();
    if (gpio == BTN_JOY || gpio == BTN_A)
    {
        if (current_time - last_btn_joy_time > DEBOUNCE_MS * 1000)
        {
            green_led_state = !green_led_state;
            gpio_put(LED_GREEN, green_led_state);
            border_style = border_style + 1;
            if (border_style > 2)
            {
                border_style = 0;
            }
            update_display(&display, square_x, square_y);
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