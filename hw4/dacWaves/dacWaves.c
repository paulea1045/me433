#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"


// SPI Defines
// We are going to use SPI 0, and allocate it to the following GPIO pins
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define SPI_PORT spi0
#define PIN_MISO 16
#define PIN_CS   17
#define PIN_SCK  18
#define PIN_MOSI 19

#define VREF 3.3

static inline void cs_select(uint cs_pin) {
    asm volatile("nop \n nop \n nop"); // FIXME
    gpio_put(cs_pin, 0);
    asm volatile("nop \n nop \n nop"); // FIXME
}

static inline void cs_deselect(uint cs_pin) {
    asm volatile("nop \n nop \n nop"); // FIXME
    gpio_put(cs_pin, 1);
    asm volatile("nop \n nop \n nop"); // FIXME
}

void writeDac(int, float);

int main()
{
    stdio_init_all();

    // SPI initialisation. This example will use SPI at 1MHz.
    spi_init(SPI_PORT, 9600);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_CS,   GPIO_FUNC_SIO);
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    
    // Chip select is active-low, so we'll initialise it to a driven-high state
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);
    // For more examples of SPI use see https://github.com/raspberrypi/pico-examples/tree/master/spi
    
    //for sines
    float t = 0.0;
    const float dt = 0.01; 
    const float frequency = 2.4; //value is from trial and error

    //for triangles
    float v = 0.0;
    float step = 0.08; // 0.08 is around 1 Hz
    bool rising = true; 

    while (true) {
        printf("Hello, worldyush!\n");

        //SIN WAVE
        sleep_ms(10);
        for (int i = 0; i < 100; i++) {
            float v = 1.65 + 1.65 * sinf(2 * M_PI * frequency * t); // 0–3.3V sine wave
            writeDac(0, v);
            sleep_ms(10);
            t += dt;
        }

        //TRIANGLE
        // writeDac(0, v);
        // sleep_ms(10); 

        // if (rising) {
        //     v += step;
        //     if (v >= VREF) {
        //         v = VREF;
        //         rising = false;
        //     }
        // } else {
        //     v -= step;
        //     if (v <= 0.0) {
        //         v = 0.0;
        //         rising = true;
        //     }
        // }
    
    }
}

void writeDac(int channel, float voltage) {
    uint8_t data[2];
    int len = 2;
    if (voltage < 0) voltage = 0;
    if (voltage > VREF) voltage = VREF;
    uint16_t value = (uint16_t)((voltage / VREF) * 1023);

    // Bit 15: 0 for DAC A
    // Bit 14: buffer bit (0 = unbuffered)
    // Bit 13: gain bit (1 = 1x gain, 0 = 2x gain)
    // Bit 12: shutdown bit (1 = active)
    // Bits 11-2: 10-bit DAC data
    // Bits 1-0: don’t care

    uint16_t command = 0;
    command |= (0b0111 << 12);
    command |= (value & 0x3FF) << 2;

    data[0] = (command >> 8) & 0xFF;
    data[1] = command & 0xFF;

    cs_select(PIN_CS);
    spi_write_blocking(SPI_PORT, data, len);
    cs_deselect(PIN_CS);
}

