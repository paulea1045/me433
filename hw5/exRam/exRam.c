#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include <math.h>

#define SPI_PORT spi0
#define PIN_MISO 16
#define PIN_SCK  18
#define PIN_MOSI 19
#define PIN_CS_DAC 17
#define PIN_CS_RAM 13

#define len 1000

//main is at the very bottom

union FloatInt {
    float f;
    uint32_t i;
};

void output_to_dac(int, float);
void upload_to_ram();
float read_from_ram();
void write_float(union FloatInt, uint16_t);
float read_float(uint16_t);

static volatile float data_array[len];

void generate_wave() {
    union FloatInt converter;
    int k = 0;
    uint16_t pos = 0;
    for (k = 0; k < len; k++) {
        data_array[k] = 1.65 * sin(k * 2 * 3.14 / len) + 1.65;
        converter.f = data_array[k];
        write_float(converter, pos);
        pos += sizeof(float);
    }
}

static inline void enable_cs(uint pin) {
    asm volatile("nop \n nop \n nop");
    gpio_put(pin, 0);
    asm volatile("nop \n nop \n nop");
}

static inline void disable_cs(uint pin) {
    asm volatile("nop \n nop \n nop");
    gpio_put(pin, 1);
    asm volatile("nop \n nop \n nop");
}

void init_spi() {
    spi_init(SPI_PORT, 10000 * 1000);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_CS_DAC, GPIO_FUNC_SIO);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);

    gpio_set_dir(PIN_CS_DAC, GPIO_OUT);
    gpio_put(PIN_CS_DAC, 1);

    gpio_init(PIN_CS_RAM);
    gpio_set_dir(PIN_CS_RAM, GPIO_OUT);
    gpio_put(PIN_CS_RAM, 1);
}

void ram_config() {
    uint8_t cmd[2];
    cmd[0] = 0b00000001;
    cmd[1] = 0b01000000;
    enable_cs(PIN_CS_RAM);
    spi_write_blocking(SPI_PORT, cmd, 2);
    disable_cs(PIN_CS_RAM);
}

void output_to_dac(int ch, float v) {
    uint8_t packet[2];
    packet[0] = 0;
    packet[0] |= (ch << 7);
    packet[0] |= (0b111 << 4);
    uint16_t raw_val = 1024 * v / 3.3;
    int bin[10] = {0};
    int idx = 0;
    int temp = raw_val;
    if (temp > 1023) temp = 1023;
    while (temp > 0) {
        bin[9 - idx] = temp % 2;
        temp /= 2;
        idx++;
    }
    packet[0] |= (bin[0] << 3);
    packet[0] |= (bin[1] << 2);
    packet[0] |= (bin[2] << 1);
    packet[0] |= (bin[3] << 0);
    packet[1] = 0;
    packet[1] |= (bin[4] << 7);
    packet[1] |= (bin[5] << 6);
    packet[1] |= (bin[6] << 5);
    packet[1] |= (bin[7] << 4);
    packet[1] |= (bin[8] << 3);
    packet[1] |= (bin[9] << 2);
    enable_cs(PIN_CS_DAC);
    spi_write_blocking(SPI_PORT, packet, 2);
    disable_cs(PIN_CS_DAC);
}

void write_float(union FloatInt u, uint16_t a) {
    uint8_t buffer[7] = {0};
    buffer[0] = 0b00000010;
    buffer[1] = (a >> 8);
    buffer[2] = a & 0xFF;
    buffer[3] = (u.i >> 24) & 0xFF;
    buffer[4] = (u.i >> 16) & 0xFF;
    buffer[5] = (u.i >> 8) & 0xFF;
    buffer[6] = (u.i) & 0xFF;

    enable_cs(PIN_CS_RAM);
    spi_write_blocking(SPI_PORT, buffer, 7);
    disable_cs(PIN_CS_RAM);
}

float read_float(uint16_t a) {
    uint8_t tx[7] = {0};
    tx[0] = 0b00000011;
    tx[1] = (a >> 8);
    tx[2] = a & 0xFF;

    uint8_t rx[7] = {0};
    enable_cs(PIN_CS_RAM);
    spi_write_read_blocking(SPI_PORT, tx, rx, 7);
    disable_cs(PIN_CS_RAM);

    union FloatInt u;
    u.i = (rx[3] << 24) | (rx[4] << 16) | (rx[5] << 8) | rx[6];
    return u.f;
}


int main() {
    stdio_init_all();
    init_spi();
    ram_config();
    generate_wave();

    while (true) {
        int m = 0;
        uint16_t index = 0;
        for (m = 0; m < len; m++) {
            float val;
            val = read_float(index);
            output_to_dac(0, val);
            index += sizeof(float);
            sleep_ms(1000 / len);
        }
    }
    return 0;
}