#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "cam.h"

// Motor H-Bridge Pin Assignments
#define IN1 16
#define IN2 17
#define IN3 18
#define IN4 19

#define PWM_WRAP 62500
#define CAM_CENTER 40

#define BASE_SPEED 20.0f
#define MAX_SPEED 90.0f
#define TURN_GAIN 1.0f

#define DEAD 4
void init_pwm_output(uint gpio_pin) {
    gpio_set_function(gpio_pin, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(gpio_pin);
    pwm_set_wrap(slice, PWM_WRAP);
    pwm_set_clkdiv(slice, 1.0f);
    pwm_set_enabled(slice, true);
}

void apply_motor_pwm(float duty_pct, uint pin_fwd, uint pin_rev) {
    if (duty_pct > 0.0f) {
        pwm_set_gpio_level(pin_fwd, 0); 
        pwm_set_gpio_level(pin_rev, (uint16_t)(PWM_WRAP * duty_pct / 100.0f));
    } else if (duty_pct < 0.0f) {
        pwm_set_gpio_level(pin_fwd, (uint16_t)(PWM_WRAP * -duty_pct / 100.0f));
        pwm_set_gpio_level(pin_rev, 0);
    } else {
        pwm_set_gpio_level(pin_fwd, 0);
        pwm_set_gpio_level(pin_rev, 0);
    }
}

int main() {
    stdio_init_all();
    init_camera_pins();

    // Set up PWM for all motor pins
    init_pwm_output(IN1);
    init_pwm_output(IN2);
    init_pwm_output(IN3);
    init_pwm_output(IN4);

    while (true) {
        //camera
        setSaveImage(1);
        while (getSaveImage()) {}
        convertImage();

        //distance from center
        int line_position = findLine(50);
        int deviation = line_position - CAM_CENTER;

        float left_output = BASE_SPEED;
        float right_output = BASE_SPEED;

        if (deviation > DEAD) {
            right_output -= TURN_GAIN * (deviation - DEAD);
        } else if (deviation < -DEAD) {
            left_output -= TURN_GAIN * (-deviation - DEAD);
        }

        //constrain PWM outputs
        if (left_output > MAX_SPEED) left_output = MAX_SPEED;
        if (left_output < 0.0f) left_output = 0.0f;

        if (right_output > MAX_SPEED) right_output = MAX_SPEED;
        if (right_output < 0.0f) right_output = 0.0f;

        //set motor speeds
        apply_motor_pwm(left_output, IN1, IN2);  
        apply_motor_pwm(right_output, IN4, IN3); 

        sleep_ms(10);  
    }

    return 0;
}
