// Example usage of the rotary dial UI on the Waveshare ESP32-P4-WIFI6-Touch-LCD-3.4C.
//
// This sketch assumes you already have Waveshare's board bring-up in place:
// lv_init(), the MIPI-DSI display driver + flush callback, the capacitive
// touch indev driver, and lv_timer_handler() running in loop() (see
// Waveshare's Arduino LVGL demo for this board). Only the parts relevant to
// wiring in the dial are shown below - replace the TODO section with your
// board's actual init code.

#include <lvgl.h>
#include "rotary_dial.h"

String dialedNumber = "";

void onDigitDialed(char digit) {
  dialedNumber += digit;
  Serial.printf("Dialed: %c  (number so far: %s)\n", digit, dialedNumber.c_str());
}

void setup() {
  Serial.begin(115200);

  // TODO: replace with your board's actual bring-up, e.g.:
  //   lv_init();
  //   <register display driver for the 800x800 MIPI-DSI panel>
  //   <register capacitive touch indev driver>

  rotary_dial_set_callback(onDigitDialed);
  rotary_dial_create(lv_scr_act());
}

void loop() {
  lv_timer_handler();
  delay(5);
}
