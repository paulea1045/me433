#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"

int main() {
    adc_init(); // init the adc module
    adc_gpio_init(26); // set ADC0 pin to be adc input instead of GPIO
    adc_select_input(0); // select to read from ADC0
    stdio_init_all();

    while (!stdio_usb_connected()) { //only include this when connecting over usb
        sleep_ms(100);
    }
    printf("Start!\n");

    // gpio_init(14); // PIN_NUM without the GP
    gpio_set_dir(14, GPIO_IN);
 
    while (1) {
        char message[100];
        scanf("%s", message);
        printf("message: %s\r\n",message);
        sleep_ms(50);
    }
}