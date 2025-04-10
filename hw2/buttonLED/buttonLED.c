#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define LED_PIN 15
#define BUTTON_PIN 16
#define LED_FLASH_TIME_MS 250

volatile int press_count = 0;  // Counter to track button presses

// Interrupt callback for button press
void button_callback(uint gpio, uint32_t events) {
    if (gpio == BUTTON_PIN && (events & GPIO_IRQ_EDGE_FALL)) {
        press_count++;  // Increment press count

        gpio_put(LED_PIN, 1);              // Turn LED on
        sleep_ms(LED_FLASH_TIME_MS);       // Keep it on for a while
        gpio_put(LED_PIN, 0);              // Turn LED off

        printf("Button pressed %d times\n", press_count);
    }
}

int main() {
    stdio_init_all();

    // Initialize LED GPIO
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);  // Start with LED off

    // Initialize button GPIO
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN); 

    // Set up interrupt on falling edge (button press)
    gpio_set_irq_enabled_with_callback(BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true, &button_callback);

    // Main loop does nothing, everything handled in interrupt
    while (1) {
        tight_loop_contents();
    }

    return 0;
}
