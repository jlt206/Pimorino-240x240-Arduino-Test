// Board bring-up + LVGL wiring for the Waveshare ESP32-P4-WIFI6-Touch-LCD-3.4C
// (800x800 round MIPI-DSI panel + capacitive touch), hosting the rotary
// dial screen from rotary_dial.h/.cpp.
//
// *** IMPORTANT — read before flashing ***
// The panel timing parameters (MIPI-DSI lane count/speed, sync porches) and
// the touch controller driver/address are board-specific. I was not able to
// verify the exact values for this board in this session (Waveshare's demo
// repo, github.com/waveshareteam/ESP32-P4-WIFI6-Touch-LCD-XC, wasn't
// browsable from here). Copy the real values from that repo's
// examples/arduino demo (or hardware/ schematic) into the two TODO sections
// below - everything else (LVGL setup, the dial screen itself) is complete
// and independent of those specifics.
//
// Library versions below (lvgl 9.3.0, Arduino_GFX 1.6.0) are confirmed from
// Waveshare's own examples/arduino/README.md for this exact board.

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <esp_heap_caps.h>
#include <lvgl.h>

#include "rotary_dial.h"

static const uint16_t SCREEN_W = 800;
static const uint16_t SCREEN_H = 800;

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

static String dialedNumber = "";

static void onDigitDialed(char digit) {
    dialedNumber += digit;
    Serial.printf("Dialed: %c  (number so far: %s)\n", digit, dialedNumber.c_str());
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

    rotary_dial_set_callback(onDigitDialed);
    rotary_dial_create(lv_screen_active());
}

void loop() {
    lv_timer_handler();
    delay(5);
}
