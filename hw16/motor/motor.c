#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"


#define IN1 17  //LEFT Motor IN1
#define IN2 16  //LEFT Motor IN2
#define IN3 18  //RIGHT Motor IN3
#define IN4 19  //RIGHT Motor IN4

#define PWM_WRAP 62500

int duty_cycle = 0; 

// Configure a pin for PWM and return its slice number (GPT initialized it for me)
uint pwm_init_gpio(uint gpio) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(gpio);
    pwm_set_wrap(slice, PWM_WRAP);
    pwm_set_clkdiv(slice, 64.f);  // optional, slows clock for easier tuning
    pwm_set_enabled(slice, true);
    return slice;
}

void set_motor(int pwm_pin, int dir_pin, int duty) {
    uint slice = pwm_gpio_to_slice_num(pwm_pin);

    if (duty > 0) {
        gpio_put(dir_pin, 0);  //Set opposite pin LOW
        pwm_set_gpio_level(pwm_pin, (PWM_WRAP * duty) / 100);
    } else if (duty < 0) {
        gpio_put(dir_pin, 1);  /Set opposite pin HIGH
        pwm_set_gpio_level(pwm_pin, (PWM_WRAP * -duty) / 100);
    } else {
        gpio_put(dir_pin, 0);
        pwm_set_gpio_level(pwm_pin, 0);
    }
}


int clamp(int val, int min, int max) {
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

int main() {
    stdio_init_all();
    gpio_init(IN1); gpio_set_dir(IN1, GPIO_OUT);
    gpio_init(IN2); gpio_set_dir(IN2, GPIO_OUT);
    gpio_init(IN3); gpio_set_dir(IN3, GPIO_OUT);
    gpio_init(IN4); gpio_set_dir(IN4, GPIO_OUT);

    //Set PWM only on IN1 (left motor) and IN3 (right motor)
    gpio_set_function(IN1, GPIO_FUNC_PWM);
    gpio_set_function(IN3, GPIO_FUNC_PWM);
    uint slice_left = pwm_gpio_to_slice_num(IN1);
    uint slice_right = pwm_gpio_to_slice_num(IN3);
    pwm_set_wrap(slice_left, PWM_WRAP);
    pwm_set_wrap(slice_right, PWM_WRAP);
    pwm_set_clkdiv(slice_left, 64.f);
    pwm_set_clkdiv(slice_right, 64.f);
    pwm_set_enabled(slice_left, true);
    pwm_set_enabled(slice_right, true);

    printf("Use '+' and '-' to change duty cycle\n");

    while (true) {
        int c = getchar_timeout_us(0);  //Non-blocking character input

        if (c == '+') {
            duty_cycle += 1;
        } else if (c == '-') {
            duty_cycle -= 1;
        } else if (c == '0') {
            duty_cycle = 0;
        }

        duty_cycle = clamp(duty_cycle, -100, 100);

        set_motor(IN1, IN2, duty_cycle);  
        set_motor(IN3, IN4, duty_cycle);  

        printf("Duty: %d%%\n", duty_cycle);
        sleep_ms(100);
    }


    return 0;
}

