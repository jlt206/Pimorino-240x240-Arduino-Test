// Board bring-up + LVGL wiring for the Waveshare ESP32-P4-WIFI6-Touch-LCD-3.4C
// (800x800 round MIPI-DSI panel + capacitive touch), hosting three
// left/right-swipeable screens: the rotary dial, a Bluetooth speed-dial
// contacts screen, and a DHT11 temperature/humidity screen.
//
// *** IMPORTANT — read before flashing ***
// The panel timing parameters (MIPI-DSI lane count/speed, sync porches),
// the touch controller driver/address, the DHT11 GPIO, and the Classic-BT
// audio module's UART pins/AT commands are all board/wiring-specific.
// I was not able to verify the exact values for this board's display/touch
// in this session (Waveshare's demo repo wasn't browsable from here) - see
// the TODOs below and in bt_audio.cpp before flashing.
//
// Library versions (lvgl 9.3.0, Arduino_GFX 1.6.0) are confirmed from
// Waveshare's own examples/arduino/README.md for this exact board.

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <esp_heap_caps.h>
#include <lvgl.h>

#include "bt_audio.h"
#include "contacts_screen.h"
#include "rotary_dial.h"
#include "weather_screen.h"

static const uint16_t SCREEN_W = 800;
static const uint16_t SCREEN_H = 800;

// TODO: confirm this pin is actually free on your build. It must avoid
// GPIO7/8 (touch I2C) and anything already claimed by the display. The
// board exposes 28 spare GPIOs on its 40-pin header - pick one you've
// physically wired the DHT11 data line to and cross-check it against
// Waveshare's silkscreen/pinout diagram.
static const int DHT11_PIN = 17;

// TODO: same caveat as above - confirm these are free before wiring the
// Classic-BT audio module's UART to them.
static const int BT_UART_RX_PIN = 18;
static const int BT_UART_TX_PIN = 19;

// ---------------------------------------------------------------------
// TODO #1: construct the real MIPI-DSI bus + panel object.
// Arduino_GFX v1.6.0's ESP32-P4 MIPI-DSI support generally looks like:
//
//   Arduino_ESP32DSIPanel *dsibus = new Arduino_ESP32DSIPanel(
//       /* lane_bit_rate_mbps */ 800, SCREEN_W,
//       /* hsync_pol */ 0, /* hsync_front_porch */ 40,
//       /* hsync_pulse_width */ 10, /* hsync_back_porch */ 40,
//       SCREEN_H,
//       /* vsync_pol */ 0, /* vsync_front_porch */ 20,
//       /* vsync_pulse_width */ 4, /* vsync_back_porch */ 20);
//   Arduino_GFX *gfx = new Arduino_DSI_Display(
//       SCREEN_W, SCREEN_H, dsibus, 0 /* rotation */, true /* auto_flush */,
//       BL_PIN, GFX_NOT_DEFINED /* reset pin */,
//       nullptr, 0 /* panel init sequence, if the panel needs one */);
//
// Replace the numbers/pins above with Waveshare's actual values, then
// uncomment and adjust this line:
Arduino_GFX *gfx = nullptr;
// ---------------------------------------------------------------------

static lv_display_t *s_disp  = nullptr;
static lv_indev_t   *s_indev = nullptr;

static lv_obj_t *s_tileview   = nullptr;
static lv_obj_t *s_tile_dial  = nullptr;
static lv_obj_t *s_tile_contacts = nullptr;
static lv_obj_t *s_tile_weather  = nullptr;

// Small dot row at the bottom of the display showing which screen is active.
static lv_obj_t *s_dots[3] = { nullptr, nullptr, nullptr };

static String s_dialed_number = "";
static uint32_t s_last_digit_ms = 0;
static const uint32_t AUTO_DIAL_PAUSE_MS = 2000;

// LVGL v9 flush callback: push the rendered area out to the panel.
static void disp_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    if (gfx) {
        int32_t w = area->x2 - area->x1 + 1;
        int32_t h = area->y2 - area->y1 + 1;
        gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)px_map, w, h);
    }
    lv_display_flush_ready(disp);
}

// ---------------------------------------------------------------------
// TODO #2: read the real capacitive touch controller here. Waveshare lists
// the board's touch I2C pins as SDA=GPIO7, SCL=GPIO8; the touch chip itself
// and its driver library weren't identifiable from this session, so use
// whichever touch library Waveshare's demo pairs with this board. Note
// their README warns that arduino-esp32 v3.2.0+'s newer "driver_ng" I2C
// driver isn't compatible with some older touch/IO-expander libraries -
// check that note if init fails.
// ---------------------------------------------------------------------
static void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data) {
    static bool touched = false;
    static int32_t last_x = 0, last_y = 0;
    // TODO: e.g. touched = touch_get_point(&last_x, &last_y);
    data->state = touched ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    data->point.x = last_x;
    data->point.y = last_y;
}

static void update_dots(uint32_t active_index) {
    for (uint32_t i = 0; i < 3; i++) {
        if (!s_dots[i]) continue;
        lv_obj_set_style_bg_color(s_dots[i], i == active_index ? lv_color_white()
                                                                 : lv_color_hex(0x555555), 0);
    }
}

static void tileview_event_cb(lv_event_t *e) {
    lv_obj_t *tv = (lv_obj_t *)lv_event_get_target(e);
    lv_obj_t *active = lv_tileview_get_tile_active(tv);
    if (active == s_tile_dial) update_dots(0);
    else if (active == s_tile_contacts) update_dots(1);
    else if (active == s_tile_weather) update_dots(2);
}

static void create_page_dots(lv_obj_t *parent) {
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, 120, 20);
    lv_obj_align(row, LV_ALIGN_BOTTOM_MID, 0, -18);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, 14, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < 3; i++) {
        lv_obj_t *dot = lv_obj_create(row);
        lv_obj_remove_style_all(dot);
        lv_obj_set_size(dot, 14, 14);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
        lv_obj_clear_flag(dot, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
        s_dots[i] = dot;
    }
    update_dots(0);
}

// Mimics an old rotary phone: once you stop dialing digits for a couple of
// seconds, the accumulated number is placed automatically. Polled from a
// regular repeating timer rather than a one-shot, since one-shot LVGL
// timers with a repeat count delete themselves after firing.
static void auto_dial_poll_cb(lv_timer_t *t) {
    if (s_dialed_number.length() > 0 && millis() - s_last_digit_ms > AUTO_DIAL_PAUSE_MS) {
        bt_audio_dial(s_dialed_number.c_str());
        Serial.printf("Auto-dialing: %s\n", s_dialed_number.c_str());
        s_dialed_number = "";
    }
}

static void onDigitDialed(char digit) {
    s_dialed_number += digit;
    s_last_digit_ms = millis();
    Serial.printf("Dialed: %c  (number so far: %s)\n", digit, s_dialed_number.c_str());
}

// The dial and the tileview both react to drags on the same tile - without
// this, rotating the dial could also be read as a swipe to the next
// screen. Suspend tileview scrolling for the duration of a dial pull.
static void onDialDragStateChanged(bool dragging) {
    if (dragging) lv_obj_clear_flag(s_tileview, LV_OBJ_FLAG_SCROLLABLE);
    else lv_obj_add_flag(s_tileview, LV_OBJ_FLAG_SCROLLABLE);
}

static void status_timer_cb(lv_timer_t *t) {
    bt_audio_poll();
    contacts_screen_update_status();
}

static void weather_timer_cb(lv_timer_t *t) {
    weather_screen_update();
}

void setup() {
    Serial.begin(115200);

    // TODO: gfx = new Arduino_DSI_Display(...); gfx->begin();
    // TODO: init the touch controller / I2C bus here.

    lv_init();
    lv_tick_set_cb([]() -> uint32_t { return millis(); });

    // Partial-render buffer: 800px wide x 60 lines, RGB565. Tune the line
    // count and MALLOC_CAP flags for your actual memory budget/performance.
    static const size_t buf_lines = 60;
    static lv_color_t *draw_buf = (lv_color_t *)heap_caps_malloc(
        SCREEN_W * buf_lines * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);

    s_disp = lv_display_create(SCREEN_W, SCREEN_H);
    lv_display_set_flush_cb(s_disp, disp_flush_cb);
    lv_display_set_buffers(s_disp, draw_buf, NULL, SCREEN_W * buf_lines * sizeof(lv_color_t),
                            LV_DISPLAY_RENDER_MODE_PARTIAL);

    s_indev = lv_indev_create();
    lv_indev_set_type(s_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(s_indev, touch_read_cb);

    bt_audio_init(BT_UART_RX_PIN, BT_UART_TX_PIN);

    // Three horizontally-swipeable, full-screen tiles: Dial | Contacts | Weather.
    s_tileview = lv_tileview_create(lv_screen_active());
    lv_obj_set_size(s_tileview, SCREEN_W, SCREEN_H);

    s_tile_dial     = lv_tileview_add_tile(s_tileview, 0, 0, LV_DIR_RIGHT);
    s_tile_contacts = lv_tileview_add_tile(s_tileview, 1, 0, LV_DIR_HOR);
    s_tile_weather  = lv_tileview_add_tile(s_tileview, 2, 0, LV_DIR_LEFT);

    rotary_dial_set_callback(onDigitDialed);
    rotary_dial_set_drag_callback(onDialDragStateChanged);
    rotary_dial_create(s_tile_dial);
    contacts_screen_create(s_tile_contacts);
    weather_screen_create(s_tile_weather, DHT11_PIN);

    create_page_dots(lv_screen_active());
    lv_obj_add_event_cb(s_tileview, tileview_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_timer_create(auto_dial_poll_cb, 200, NULL);
    lv_timer_create(status_timer_cb, 1000, NULL);
    lv_timer_create(weather_timer_cb, 3000, NULL);
}

void loop() {
    lv_timer_handler();
    delay(5);
}
