#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "font.h"
#include "ssd1306.h"
#include "hardware/i2c.h"

#define I2C_PORT i2c0
#define I2C_SDA 0
#define I2C_SCL 1
#define LED_PIN PICO_DEFAULT_LED_PIN 
#define ADDR 0x68

// config registers
#define CONFIG 0x1A
#define GYRO_CONFIG 0x1B
#define ACCEL_CONFIG 0x1C
#define PWR_MGMT_1 0x6B
#define PWR_MGMT_2 0x6C
// sensor data registers:
#define ACCEL_XOUT_H 0x3B
#define ACCEL_XOUT_L 0x3C
#define ACCEL_YOUT_H 0x3D
#define ACCEL_YOUT_L 0x3E
#define ACCEL_ZOUT_H 0x3F
#define ACCEL_ZOUT_L 0x40
#define TEMP_OUT_H   0x41
#define TEMP_OUT_L   0x42
#define GYRO_XOUT_H  0x43
#define GYRO_XOUT_L  0x44
#define GYRO_YOUT_H  0x45
#define GYRO_YOUT_L  0x46
#define GYRO_ZOUT_H  0x47
#define GYRO_ZOUT_L  0x48
#define WHO_AM_I     0x75

void imu_init();
void draw_vector(int16_t ax, int16_t ay);
void imu_read(int16_t *ax, int16_t *ay, int16_t *az, int16_t *gx, int16_t *gy, int16_t *gz);

int main() {
    stdio_init_all();

    //HW7
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    ssd1306_setup();
    ssd1306_clear();
    ssd1306_update();
    //HW7
    imu_init(); 

    int16_t ax, ay, az, gx, gy, gz;

    while (true) {
        imu_read(&ax, &ay, &az, &gx, &gy, &gz);
        draw_vector(ax, ay);
        sleep_ms(25);
    }
}

void imu_init() {
    uint8_t buf[] = {PWR_MGMT_1, 0x00};
    i2c_write_blocking(I2C_PORT, ADDR, buf, 2, false);
    sleep_ms(100);  
}

void imu_read(int16_t *ax, int16_t *ay, int16_t *az,
                  int16_t *gx, int16_t *gy, int16_t *gz) {
    uint8_t reg = ACCEL_XOUT_H;
    uint8_t data[14]; //sequential read of 14 bytes

    i2c_write_blocking(I2C_PORT, ADDR, &reg, 1, true);
    i2c_read_blocking(I2C_PORT, ADDR, data, 14, false);

    *ax = (int16_t)(data[0] << 8 | data[1]);
    *ay = (int16_t)(data[2] << 8 | data[3]);
    *az = (int16_t)(data[4] << 8 | data[5]);
    *gx = (int16_t)(data[8] << 8 | data[9]);
    *gy = (int16_t)(data[10] << 8 | data[11]);
    *gz = (int16_t)(data[12] << 8 | data[13]);
}


void draw_vector(int16_t ax, int16_t ay) {
    // Slightly reduced scale for stability
    float fx = ax * 0.00005f;
    float fy = ay * 0.00005f;

    int cx = 64;
    int cy = 16;
    int dx = (int)(-fx * 64); 
    int dy = (int)(fy * 32);

    int x0 = cx; //unncessary but for naming conventions 
    int y0 = cy;
    int x1 = cx + dx;
    int y1 = cy + dy;

    int dx_line = x1 - x0;
    int dy_line = y1 - y0;

    int sx;
    int sy;

    if (dx_line < 0) {
        sx = -1;
        dx_line = -dx_line;
    } else {
        sx = 1;
    }

    if (dy_line < 0) {
        sy = -1;
        dy_line = -dy_line;
    } else {
        sy = 1;
    }

    int err = dx_line - dy_line;

    ssd1306_clear();

    while (1) {
        ssd1306_drawPixel(x0, y0, 1);
        if (x0 == x1 && y0 == y1) {
            break;
        }

        int e2 = 2 * err;
        if (e2 > -dy_line) {
            err = err - dy_line;
            x0 = x0 + sx;
        }
        if (e2 < dx_line) {
            err = err + dx_line;
            y0 = y0 + sy;
        }
    }

    ssd1306_update();
}

