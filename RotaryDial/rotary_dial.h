// Vintage rotary-phone dial UI for the Waveshare ESP32-P4-WIFI6-Touch-LCD-3.4C
// (3.4" 800x800 round MIPI-DSI panel + capacitive touch).
//
// This module only builds the dial screen. It assumes your own board bring-up
// code has already called lv_init() and registered the display + touch
// indev drivers for this panel (e.g. Waveshare's LVGL demo project) before
// rotary_dial_create() runs, and that you are calling lv_timer_handler() in
// loop() as usual.
//
// Written against LVGL v8.3 (uses the generic lv_obj_set_style_transform_angle
// / transform_pivot style properties to rotate the dial disc, and the
// lv_canvas API to draw it). For LVGL v9, swap these for the lv_image_* /
// lv_obj_set_style_transform_rotation equivalents.
//
// Fonts: labels use &lv_font_montserrat_28 and &lv_font_montserrat_16.
// Enable LV_FONT_MONTSERRAT_28 and LV_FONT_MONTSERRAT_16 in lv_conf.h if they
// aren't already on.
#pragma once

#include <lvgl.h>

// Called once per completed dial pull (after the disc springs back to rest).
// `digit` is '0'-'9'.
typedef void (*rotary_dial_digit_cb_t)(char digit);

// Builds the fixed faceplate + rotating finger-hole disc as a child of
// `parent`, sized to fill an 800x800 round display. Returns the root object.
lv_obj_t *rotary_dial_create(lv_obj_t *parent);

// Registers the digit-dialed callback. Pass NULL to clear it.
void rotary_dial_set_callback(rotary_dial_digit_cb_t cb);
