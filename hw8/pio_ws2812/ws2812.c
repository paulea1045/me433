#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/pwm.h"
#include "ws2812.pio.h"

#define NUM_PIXELS 4
#define IS_RGBW false
#define WS2812_PIN 2
#define SERVO_PIN 15

#define FRAME_DELAY_MS 20
#define SWEEP_DURATION_MS 5000
#define TOTAL_FRAMES (SWEEP_DURATION_MS / FRAME_DELAY_MS)

typedef struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
} wsColor;

wsColor HSBtoRGB(float hue, float sat, float brightness) {
    float red = 0.0, green = 0.0, blue = 0.0;
    if (sat == 0.0) {
        red = green = blue = brightness;
    } else {
        if (hue >= 360.0f) hue -= 360.0f;
        int slice = hue / 60.0f;
        float hue_frac = (hue / 60.0f) - slice;
        float aa = brightness * (1.0f - sat);
        float bb = brightness * (1.0f - sat * hue_frac);
        float cc = brightness * (1.0f - sat * (1.0f - hue_frac));

        //GPT helped write switch cases
        switch (slice) {
            case 0: red = brightness; green = cc; blue = aa; break;
            case 1: red = bb; green = brightness; blue = aa; break;
            case 2: red = aa; green = brightness; blue = cc; break;
            case 3: red = aa; green = bb; blue = brightness; break;
            case 4: red = cc; green = aa; blue = brightness; break;
            case 5: red = brightness; green = aa; blue = bb; break;
            default: red = green = blue = 0.0; break;
        }
    }
    wsColor c;
    c.r = red * 255;
    c.g = green * 255;
    c.b = blue * 255;
    return c;
}

static inline void put_pixel(PIO pio, uint sm, uint32_t pixel_grb) {
    pio_sm_put_blocking(pio, sm, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}

void setup_pwm_for_servo(uint pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(pin);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 125.0f); // 1 MHz clock = 1us resolution
    pwm_config_set_wrap(&config, 20000);    // 20ms = 50Hz
    pwm_init(slice_num, &config, true);
}

void set_servo_angle(uint pin, float angle_deg) {
    if (angle_deg < 0) angle_deg = 0;
    if (angle_deg > 180) angle_deg = 180;
    uint slice = pwm_gpio_to_slice_num(pin);
    uint channel = pwm_gpio_to_channel(pin);
    uint pulse_width_us = 500 + (angle_deg / 180.0f) * 2000; // 500–2500 µs
    pwm_set_chan_level(slice, channel, pulse_width_us);
}


int main() {
    stdio_init_all();
    sleep_ms(1000);  

    //LED init
    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);

    //servo init
    setup_pwm_for_servo(SERVO_PIN);

    while (1) {
        //forward sweep
        for (int t = 0; t < TOTAL_FRAMES; ++t) {
            float progress = (float)t / TOTAL_FRAMES;
            float angle = progress * 180.0f;

            //servo
            set_servo_angle(SERVO_PIN, angle);

            //LED rainbow
            float base_hue = 360.0f * progress;
            for (int i = 0; i < NUM_PIXELS; ++i) {
                float hue = base_hue + i * 90.0f;
                if (hue >= 360.0f) hue -= 360.0f;
                wsColor c = HSBtoRGB(hue, 1.0f, 0.05f);  // brightness = 0.05
                put_pixel(pio, sm, urgb_u32(c.r, c.g, c.b));
            }

            sleep_ms(FRAME_DELAY_MS);
        }

        //backward sweep
        for (int t = 0; t < TOTAL_FRAMES; ++t) {
            float progress = (float)t / TOTAL_FRAMES;
            float angle = 180.0f - progress * 180.0f;
            
            //servo
            set_servo_angle(SERVO_PIN, angle);

            //LED rainbow
            float base_hue = 360.0f * progress;
            for (int i = 0; i < NUM_PIXELS; ++i) {
                float hue = base_hue + i * 90.0f;
                if (hue >= 360.0f) hue -= 360.0f;
                wsColor c = HSBtoRGB(hue, 1.0f, 0.05f);
                put_pixel(pio, sm, urgb_u32(c.r, c.g, c.b));
            }

            sleep_ms(FRAME_DELAY_MS);
        }
    }

        return 0;
}
