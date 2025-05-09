#include <stdio.h>
#include "pico/stdlib.h"
#include "font.h"
#include "ssd1306.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
//I didn't do ADC. I just had it print the FPS and chickens

// I2C defines
// This example will use I2C0 on GPIO8 (SDA) and GPIO9 (SCL) running at 400KHz.
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define I2C_PORT i2c0
#define I2C_SDA 8
#define I2C_SCL 9

#define LED_PIN PICO_DEFAULT_LED_PIN  // built in led

void drawLetter(int x, int y, char c);
void drawMessage(int x, int y, char*m);

int main()
{
    stdio_init_all();

    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400*1000);

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    // For more examples of I2C use see https://github.com/raspberrypi/pico-examples/tree/master/i2c

    ssd1306_setup();
    ssd1306_clear();
    ssd1306_update();


    int counter = 1;

    while (true) {
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
        sleep_ms(100);

        unsigned int t1 = to_us_since_boot(get_absolute_time());  

        ssd1306_clear();

        // Convert counter to string and draw it
        char message[16];
        sprintf(message, "%d chickens", counter);
        drawMessage(0, 0, message);

        // Increment and loop the counter
        counter++;
        if (counter > 10) {
            counter = 1;
        }

        unsigned int t2 = to_us_since_boot(get_absolute_time());  
        unsigned int t_elapsed = t2 - t1; // process time in microseconds

        float fps = (float)(1/((float)(t_elapsed/1000000.0)));// turn time to frames per second
        char fps_message[32];
        sprintf(fps_message, "FPS: %.2f", fps);
        drawMessage(0, 24, fps_message);

        ssd1306_update();

        printf("Total time: %llu ms, FPS: %.2f\n", t_elapsed, fps);

        gpio_put(PICO_DEFAULT_LED_PIN, 0);
        sleep_ms(500);  // Delay so numbers change more visibly

    }

}


void drawMessage(int x, int y, char*m){
    int i = 0;
    while (m[i] != 0){
        drawLetter(x+i*5,y,m[i]);
        i++;
    }
}

void drawLetter(int x, int y, char c){
    int j;
    for(j = 0;j<5;j++){
        char col = ASCII[c-0x20][j];
        int i ;
        for(i = 0;i<8;i++){
            char bit = (col >> i)&0b1;
            ssd1306_drawPixel(x+j,y+i, bit);
        }
    }
}