#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"

#define LED_PIN 15
#define ADC_PIN 26

#define CMD_READ_VOLTAGE 0
#define CMD_LED_ON 1
#define CMD_LED_OFF 2

//Core 1 handles = LED and ADC
void core1_entry() {
    adc_init();
    adc_gpio_init(ADC_PIN); // Initialize ADC pin
    adc_select_input(0); // Read from ADC0 (GPIO26)

    gpio_init(LED_PIN); // Initialize LED pin
    gpio_set_dir(LED_PIN, GPIO_OUT);

    while (1) {
        uint32_t command = multicore_fifo_pop_blocking();

        switch (command) {
            case CMD_READ_VOLTAGE: {
                uint16_t adc_raw = adc_read();
                multicore_fifo_push_blocking(adc_raw); // Return the ADC result
                break;
            }
            case CMD_LED_ON:
                gpio_put(LED_PIN, true);
                break;

            case CMD_LED_OFF:
                gpio_put(LED_PIN, false);
                break;

            default:
                break;
        }
    }
}

//Core 0 = comms
int main() {
    stdio_init_all();

    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }

    puts("Multicore Communication Started");

    multicore_launch_core1(core1_entry);
    puts("Core 1 Launched");

    while (1) {
        uint32_t command;

        printf("\nEnter command (0=Read Voltage, 1=LED ON, 2=LED OFF): ");
        scanf("%u", &command);

        multicore_fifo_push_blocking(command);

        switch (command) {
            case CMD_READ_VOLTAGE: {
                uint16_t adc_result = multicore_fifo_pop_blocking();
                float voltage = (3.3f * adc_result) / 4095.0f;
                printf("Voltage on A0: %.2f V\n", voltage);
                break;
            }

            case CMD_LED_ON:
                puts("Turning ON LED on GP15");
                break;

            case CMD_LED_OFF:
                puts("Turning OFF LED on GP15");
                break;

            default:
                puts("Invalid command.");
                break;
        }

        sleep_ms(100);
    }
}
