#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "font.h"
#include "ssd1306.h"
#include "hardware/i2c.h"

#define I2C_PORT i2c0
#define I2C_SDA 8
#define I2C_SCL 9
#define LED_PIN PICO_DEFAULT_LED_PIN

void drawCharWithSpacing(int x, int y, char c);

int main() {
    stdio_init_all();

    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    ssd1306_setup();
    ssd1306_clear();
    ssd1306_update();

    const char *message = "  yosadfasdjfasd;f this is from chat gpt";
    int scroll = 0;

    while (true) {
        gpio_put(LED_PIN, 1);
        ssd1306_clear();

        int x = -scroll;
        int i = 0;
        while (x < 128 && message[i] != 0) {
            if (x > -6) {  // only draw if the character is at least partially visible
                drawCharWithSpacing(x, 0, message[i]);
            }
            x += 6;  // 5 pixels for glyph + 1 space
            i++;
        }

        ssd1306_update();
        gpio_put(LED_PIN, 0);
        sleep_ms(50);

        scroll = scroll+3;
        if (scroll > (int)strlen(message) * 6) {
            scroll = 0;
        }
    }
}

void drawCharWithSpacing(int x, int y, char c) {
    for (int j = 0; j < 5; j++) {
        char col = ASCII[c - 0x20][j];
        for (int i = 0; i < 8; i++) {
            char bit = (col >> i) & 0b1;
            ssd1306_drawPixel(x + j, y + i, bit);
        }
    }
}
