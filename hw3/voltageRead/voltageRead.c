#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"

#define PIN_NUM 15
#define BUTTON_PIN 16

void button_callback(uint gpio, uint32_t events) {
    if (gpio == BUTTON_PIN && (events & GPIO_IRQ_EDGE_FALL)) {

        gpio_put(PIN_NUM, 0);              // Turn LED off
        uint16_t result = adc_read();
        printf("Voltage value: %d\n", result);
    }
}

int main() {
    stdio_init_all();
    gpio_init(PIN_NUM); // PIN_NUM without the GP
    gpio_set_dir(PIN_NUM, GPIO_OUT);
    gpio_get(PIN_NUM);
    gpio_put(PIN_NUM, 0); //off

    adc_init(); // init the adc module
    adc_gpio_init(26); // set ADC0 pin to be adc input instead of GPIO
    adc_select_input(0); // select to read from ADC0

    // uint16_t result = adc_read();

    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN); 

    gpio_set_irq_enabled_with_callback(BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true, &button_callback);



    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }
    printf("Start!\n");
    gpio_put(PIN_NUM, 1); //on
 
    while (1) {
        char message[100];
        scanf("%s", message);
        printf("message: %s\r\n",message);
        sleep_ms(50);
    }
}