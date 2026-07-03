#include "weather_screen.h"

#include <DHT.h>

static DHT *s_dht = nullptr;
static lv_obj_t *s_temp_label = nullptr;
static lv_obj_t *s_humidity_label = nullptr;
static lv_obj_t *s_status_label = nullptr;

lv_obj_t *weather_screen_create(lv_obj_t *parent, int dht_pin) {
    static DHT dht(dht_pin, DHT11);
    s_dht = &dht;
    s_dht->begin();

    lv_obj_t *root = lv_obj_create(parent);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(root, lv_color_hex(0x0F1A2B), 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(root);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x8AA0C8), 0);
    lv_label_set_text(title, "TEMPERATURE / HUMIDITY");
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -140);

    s_temp_label = lv_label_create(root);
    lv_obj_set_style_text_font(s_temp_label, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(s_temp_label, lv_color_white(), 0);
    lv_label_set_text(s_temp_label, "-- C");
    lv_obj_align(s_temp_label, LV_ALIGN_CENTER, 0, -40);

    s_humidity_label = lv_label_create(root);
    lv_obj_set_style_text_font(s_humidity_label, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(s_humidity_label, lv_color_white(), 0);
    lv_label_set_text(s_humidity_label, "-- % RH");
    lv_obj_align(s_humidity_label, LV_ALIGN_CENTER, 0, 40);

    s_status_label = lv_label_create(root);
    lv_obj_set_style_text_font(s_status_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_status_label, lv_color_hex(0x8AA0C8), 0);
    lv_label_set_text(s_status_label, "");
    lv_obj_align(s_status_label, LV_ALIGN_CENTER, 0, 140);

    return root;
}

void weather_screen_update(void) {
    if (!s_dht) return;

    float temp_c = s_dht->readTemperature();
    float humidity = s_dht->readHumidity();

    if (isnan(temp_c) || isnan(humidity)) {
        lv_label_set_text(s_status_label, "Sensor read failed - check wiring");
        return;
    }

    lv_label_set_text(s_status_label, "");
    lv_label_set_text_fmt(s_temp_label, "%.1f C", temp_c);
    lv_label_set_text_fmt(s_humidity_label, "%.0f %% RH", humidity);
}
