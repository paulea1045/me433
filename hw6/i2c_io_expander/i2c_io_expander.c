#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define I2C_PORT i2c0
#define I2C_SDA 8
#define I2C_SCL 9

// MCP23008 I2C address (with A0, A1, A2 grounded)
#define MCP23008_ADDR 0x20

// MCP23008 Registers
#define IODIR    0x00  // I/O direction register
#define GPIO     0x09  // General purpose I/O port
#define OLAT     0x0A  // Output latch register

#define LED_PIN PICO_DEFAULT_LED_PIN  // built in led

//mcp write and read functions
void mcp23008_write(uint8_t reg, uint8_t value) {
    uint8_t buf[2] = {reg, value};
    i2c_write_blocking(I2C_PORT, MCP23008_ADDR, buf, 2, false);
}

uint8_t mcp23008_read(uint8_t reg) {
    uint8_t buf;
    i2c_write_blocking(I2C_PORT, MCP23008_ADDR, &reg, 1, true);
    i2c_read_blocking(I2C_PORT, MCP23008_ADDR, &buf, 1, false);
    return buf;
}


// MCP23008 initialization
void mcp23008_init() {
    // Set GP7 as output (0) and GP0 as input (1), rest as input
    mcp23008_write(IODIR, 0x7F);
    mcp23008_write(OLAT, 0x00);
}

int main() {
    stdio_init_all();
    
    // I2C at 400 kHz heartbeat
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    //  built-in LED for heartbeat
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    mcp23008_init();

    while (true) {
        // heartbeat led
        gpio_put(LED_PIN, 1);
        sleep_ms(100);
        gpio_put(LED_PIN, 0);
        sleep_ms(100);

        
        uint8_t gpio_state = mcp23008_read(GPIO);
        bool button_pressed = !(gpio_state & 0x01);  

        if (button_pressed) {
            mcp23008_write(OLAT, 0x80);  
            printf("Button pressed: LED ON\n");
        } else {
            mcp23008_write(OLAT, 0x00);  
            printf("Button not pressed: LED OFF\n");
        }
    }
}
