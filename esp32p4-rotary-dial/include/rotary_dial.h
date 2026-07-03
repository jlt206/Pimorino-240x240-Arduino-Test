// Vintage rotary-phone dial UI for the Waveshare ESP32-P4-WIFI6-Touch-LCD-3.4C
// (3.4" 800x800 round MIPI-DSI panel + capacitive touch).
//
// This module only builds the dial screen. It assumes your own board
// bring-up code (see src/main.cpp) has already called lv_init() and
// registered the display + touch indev drivers for this panel before
// rotary_dial_create() runs, and that lv_timer_handler() is called in
// loop() as usual.
//
// Written against LVGL v9.3.0 (matches the version Waveshare's own Arduino
// notes for this board specify). The rotating disc is a plain lv_obj (not
// an image/canvas): its "holes" are child objects, and the whole disc is
// spun live via the generic lv_obj_set_style_transform_rotation() +
// transform_pivot_x/y style properties, which apply to any widget
// (including its children) since this is how LVGL composites objects
// through its layer-based renderer.
//
// Fonts: labels use &lv_font_montserrat_28 and &lv_font_montserrat_16.
// This project's include/lv_conf.h already enables both.
#pragma once

#include <lvgl.h>

// Called once per completed dial pull (after the disc springs back to rest).
// `digit` is '0'-'9'.
typedef void (*rotary_dial_digit_cb_t)(char digit);

// Called with `true` the moment a finger hole is grabbed, and `false` once
// the disc has released (either sprung back to rest, or the touch was lost
// without ever grabbing a hole). If this screen lives inside an
// lv_tileview, wire this to pause tileview swiping while `true`, so
// dialing a digit can't also be read as a swipe to the next screen.
typedef void (*rotary_dial_drag_cb_t)(bool dragging);

// Builds the fixed faceplate + rotating finger-hole disc as a child of
// `parent`, sized to fill an 800x800 round display. Returns the root object.
lv_obj_t *rotary_dial_create(lv_obj_t *parent);

// Registers the digit-dialed callback. Pass NULL to clear it.
void rotary_dial_set_callback(rotary_dial_digit_cb_t cb);

// Registers the drag-state callback. Pass NULL to clear it.
void rotary_dial_set_drag_callback(rotary_dial_drag_cb_t cb);
