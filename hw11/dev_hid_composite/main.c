/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "hardware/gpio.h"

#include "bsp/board_api.h"
#include "tusb.h"

#include "usb_descriptors.h"


#define UP_PIN 15
#define LEFT_PIN 14
#define RIGHT_PIN 16
#define DOWN_PIN 13
#define CIRCLE_PIN 17
#define LED_PIN 18


enum  {
  BLINK_NOT_MOUNTED = 250,
  BLINK_MOUNTED = 1000,
  BLINK_SUSPENDED = 2500,
};

static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;

void led_blinking_task(void);
void hid_task(void);
int8_t delta_x = 0;
int8_t delta_y = 0;

bool circle_mode = 0;
bool toggle_pressed = false;

void update_circle(void);

/*------------- MAIN -------------*/
int main(void)
{
  board_init();

  // init device stack on configured roothub port
  tud_init(BOARD_TUD_RHPORT);

  if (board_init_after_tusb) {
    board_init_after_tusb();
  }

  gpio_init(UP_PIN);
  gpio_set_dir(UP_PIN, GPIO_IN);
  gpio_pull_up(UP_PIN);

  gpio_init(LEFT_PIN);
  gpio_set_dir(LEFT_PIN, GPIO_IN);
  gpio_pull_up(LEFT_PIN);

  gpio_init(RIGHT_PIN);
  gpio_set_dir(RIGHT_PIN, GPIO_IN);
  gpio_pull_up(RIGHT_PIN);

  gpio_init(DOWN_PIN);
  gpio_set_dir(DOWN_PIN, GPIO_IN);
  gpio_pull_up(DOWN_PIN);

  gpio_init(CIRCLE_PIN);
  gpio_set_dir(CIRCLE_PIN, GPIO_IN);
  gpio_pull_up(CIRCLE_PIN);

  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);
  gpio_put(LED_PIN, 0); // Off by default



  uint32_t up_timer = 0, down_timer = 0, left_timer = 0, right_timer = 0;

  while (1)
  {
    tud_task(); 
    led_blinking_task();
    hid_task();

    uint32_t now = board_millis();

    if(circle_mode) {
      update_circle();
    } else {
      //y-direction
      if (!gpio_get(UP_PIN)) {
        up_timer += 10;
        if (up_timer < 500) delta_y = -2;
        else if (up_timer < 1500) delta_y = -5;
        else delta_y = -10;
      } else if (!gpio_get(DOWN_PIN)) {
        down_timer += 10;
        if (down_timer < 500) delta_y = 2;
        else if (down_timer < 1500) delta_y = 5;
        else delta_y = 10;
      } else {
        up_timer = 0;
        down_timer = 0;
        delta_y = 0;
      }

      //x-direction
      if (!gpio_get(LEFT_PIN)) {
        left_timer += 10;
        if (left_timer < 500) delta_x = -2;
        else if (left_timer < 1500) delta_x = -5;
        else delta_x = -10;
      } else if (!gpio_get(RIGHT_PIN)) {
        right_timer += 10;
        if (right_timer < 500) delta_x = 2;
        else if (right_timer < 1500) delta_x = 5;
        else delta_x = 10;
      } else {
        left_timer = 0;
        right_timer = 0;
        delta_x = 0;
      }
    }

    if (!gpio_get(CIRCLE_PIN) && !toggle_pressed) {
      circle_mode = !circle_mode;
      toggle_pressed = true;
    } else if (gpio_get(CIRCLE_PIN)) {
      toggle_pressed = false;
    }
    if (circle_mode) {
      gpio_put(LED_PIN, 1);
    } else {
      gpio_put(LED_PIN, 0);
    }

    // Small delay for stability (approx. loop timing)
    sleep_ms(10);
  }
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
  blink_interval_ms = BLINK_MOUNTED;
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
  blink_interval_ms = BLINK_NOT_MOUNTED;
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
  (void) remote_wakeup_en;
  blink_interval_ms = BLINK_SUSPENDED;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
  blink_interval_ms = tud_mounted() ? BLINK_MOUNTED : BLINK_NOT_MOUNTED;
}

//--------------------------------------------------------------------+
// USB HID
//--------------------------------------------------------------------+

static void send_hid_report(uint8_t report_id, uint32_t btn)
{
  // skip if hid is not ready yet
  if ( !tud_hid_ready() ) return;

  switch(report_id)
  {
    case REPORT_ID_KEYBOARD:
    {
      // use to avoid send multiple consecutive zero report for keyboard
      static bool has_keyboard_key = false;

      if ( btn )
      {
        uint8_t keycode[6] = { 0 };
        keycode[0] = HID_KEY_A;

        tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, keycode);
        has_keyboard_key = true;
      }else
      {
        // send empty key report if previously has key pressed
        if (has_keyboard_key) tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, NULL);
        has_keyboard_key = false;
      }
    }
    break;

    case REPORT_ID_MOUSE: //Changed this to rid of constant delta and change delta --> delta_x and delta_y
    {

      // no button, right + down, no scroll, no pan
      tud_hid_mouse_report(REPORT_ID_MOUSE, 0x00, delta_x, delta_y, 0, 0);
    }
    break;

    case REPORT_ID_CONSUMER_CONTROL:
    {
      // use to avoid send multiple consecutive zero report
      static bool has_consumer_key = false;

      if ( btn )
      {
        // volume down
        uint16_t volume_down = HID_USAGE_CONSUMER_VOLUME_DECREMENT;
        tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &volume_down, 2);
        has_consumer_key = true;
      }else
      {
        // send empty key report (release key) if previously has key pressed
        uint16_t empty_key = 0;
        if (has_consumer_key) tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &empty_key, 2);
        has_consumer_key = false;
      }
    }
    break;

    case REPORT_ID_GAMEPAD:
    {
      // use to avoid send multiple consecutive zero report for keyboard
      static bool has_gamepad_key = false;

      hid_gamepad_report_t report =
      {
        .x   = 0, .y = 0, .z = 0, .rz = 0, .rx = 0, .ry = 0,
        .hat = 0, .buttons = 0
      };

      if ( btn )
      {
        report.hat = GAMEPAD_HAT_UP;
        report.buttons = GAMEPAD_BUTTON_A;
        tud_hid_report(REPORT_ID_GAMEPAD, &report, sizeof(report));

        has_gamepad_key = true;
      }else
      {
        report.hat = GAMEPAD_HAT_CENTERED;
        report.buttons = 0;
        if (has_gamepad_key) tud_hid_report(REPORT_ID_GAMEPAD, &report, sizeof(report));
        has_gamepad_key = false;
      }
    }
    break;

    default: break;
  }
}

// Every 10ms, we will sent 1 report for each HID profile (keyboard, mouse etc ..)
// tud_hid_report_complete_cb() is used to send the next report after previous one is complete
void hid_task(void)
{
  // Poll every 10ms
  const uint32_t interval_ms = 10;
  static uint32_t start_ms = 0;

  if ( board_millis() - start_ms < interval_ms) return; // not enough time
  start_ms += interval_ms;

  uint32_t const btn = board_button_read();

  // Remote wakeup
  if ( tud_suspended() && btn )
  {
    // Wake up host if we are in suspend mode
    // and REMOTE_WAKEUP feature is enabled by host
    tud_remote_wakeup();
  }else
  {
    // Send the 1st of report chain, the rest will be sent by tud_hid_report_complete_cb()
    send_hid_report(REPORT_ID_MOUSE, btn); //changed to mouse
  }
}

// Invoked when sent REPORT successfully to host
// Application can use this to send the next report
// Note: For composite reports, report[0] is report ID
void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, uint16_t len)
{
  (void) instance;
  (void) len;

  uint8_t next_report_id = report[0] + 1u;

  if (next_report_id < REPORT_ID_COUNT)
  {
    send_hid_report(next_report_id, board_button_read());
  }
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
  // TODO not Implemented
  (void) instance;
  (void) report_id;
  (void) report_type;
  (void) buffer;
  (void) reqlen;

  return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
  (void) instance;

  if (report_type == HID_REPORT_TYPE_OUTPUT)
  {
    // Set keyboard LED e.g Capslock, Numlock etc...
    if (report_id == REPORT_ID_KEYBOARD)
    {
      // bufsize should be (at least) 1
      if ( bufsize < 1 ) return;

      uint8_t const kbd_leds = buffer[0];

      if (kbd_leds & KEYBOARD_LED_CAPSLOCK)
      {
        // Capslock On: disable blink, turn led on
        blink_interval_ms = 0;
        board_led_write(true);
      }else
      {
        // Caplocks Off: back to normal blink
        board_led_write(false);
        blink_interval_ms = BLINK_MOUNTED;
      }
    }
  }
}

void update_circle() {

  static float theta = 0;
  float radius = 10.0;
  delta_x = (int8_t)(radius * cosf(theta));
  delta_y = (int8_t)(radius * sinf(theta));

  theta += 0.1f; 
  if (theta >= 2 * M_PI) {
    theta -= 2 * M_PI;
  }
}


//--------------------------------------------------------------------+
// BLINKING TASK //KEPT THE SAME AS EXAMPLE
//--------------------------------------------------------------------+
void led_blinking_task(void)
{
  static uint32_t start_ms = 0;
  static bool led_state = false;

  // blink is disabled
  if (!blink_interval_ms) return;

  // Blink every interval ms
  if ( board_millis() - start_ms < blink_interval_ms) return; // not enough time
  start_ms += blink_interval_ms;

  board_led_write(led_state);
  led_state = 1 - led_state; // toggle
}


