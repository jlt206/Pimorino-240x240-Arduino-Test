#include "weather_screen.h"

#include <DHT.h>

static DHT *s_dht = nullptr;

// Indoor (DHT11) widgets.
static lv_obj_t *s_indoor_temp_label = nullptr;
static lv_obj_t *s_indoor_humidity_label = nullptr;
static lv_obj_t *s_indoor_status_label = nullptr;

// Local weather (web) widgets.
static lv_obj_t *s_web_condition_label = nullptr;
static lv_obj_t *s_web_temp_label = nullptr;
static lv_obj_t *s_web_humidity_label = nullptr;
static lv_obj_t *s_web_status_label = nullptr;

static lv_obj_t *make_section_title(lv_obj_t *parent, const char *text, lv_coord_t y_offset) {
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0x8AA0C8), 0);
    lv_label_set_text(label, text);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, y_offset);
    return label;
}

lv_obj_t *weather_screen_create(lv_obj_t *parent, int dht_pin) {
    static DHT dht(dht_pin, DHT11);
    s_dht = &dht;
    s_dht->begin();

    lv_obj_t *root = lv_obj_create(parent);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(root, lv_color_hex(0x0F1A2B), 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    // ---- Indoor (DHT11) ----
    make_section_title(root, "INDOOR (DHT11)", -300);

    s_indoor_temp_label = lv_label_create(root);
    lv_obj_set_style_text_font(s_indoor_temp_label, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(s_indoor_temp_label, lv_color_white(), 0);
    lv_label_set_text(s_indoor_temp_label, "-- C");
    lv_obj_align(s_indoor_temp_label, LV_ALIGN_CENTER, 0, -245);

    s_indoor_humidity_label = lv_label_create(root);
    lv_obj_set_style_text_font(s_indoor_humidity_label, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(s_indoor_humidity_label, lv_color_white(), 0);
    lv_label_set_text(s_indoor_humidity_label, "-- % RH");
    lv_obj_align(s_indoor_humidity_label, LV_ALIGN_CENTER, 0, -195);

    s_indoor_status_label = lv_label_create(root);
    lv_obj_set_style_text_font(s_indoor_status_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_indoor_status_label, lv_color_hex(0xE0A030), 0);
    lv_label_set_text(s_indoor_status_label, "");
    lv_obj_align(s_indoor_status_label, LV_ALIGN_CENTER, 0, -150);

    // ---- Local weather (web) ----
    make_section_title(root, "LOCAL WEATHER TODAY", -60);

    s_web_condition_label = lv_label_create(root);
    lv_obj_set_style_text_font(s_web_condition_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_web_condition_label, lv_color_hex(0xCCCCCC), 0);
    lv_label_set_text(s_web_condition_label, "--");
    lv_obj_align(s_web_condition_label, LV_ALIGN_CENTER, 0, -5);

    s_web_temp_label = lv_label_create(root);
    lv_obj_set_style_text_font(s_web_temp_label, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(s_web_temp_label, lv_color_white(), 0);
    lv_label_set_text(s_web_temp_label, "-- C");
    lv_obj_align(s_web_temp_label, LV_ALIGN_CENTER, 0, 55);

    s_web_humidity_label = lv_label_create(root);
    lv_obj_set_style_text_font(s_web_humidity_label, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(s_web_humidity_label, lv_color_white(), 0);
    lv_label_set_text(s_web_humidity_label, "-- % RH");
    lv_obj_align(s_web_humidity_label, LV_ALIGN_CENTER, 0, 105);

    s_web_status_label = lv_label_create(root);
    lv_obj_set_style_text_font(s_web_status_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_web_status_label, lv_color_hex(0xE0A030), 0);
    lv_label_set_text(s_web_status_label, "Waiting for Wi-Fi...");
    lv_obj_align(s_web_status_label, LV_ALIGN_CENTER, 0, 155);

    return root;
}

void weather_screen_update(void) {
    if (!s_dht) return;

    float temp_c = s_dht->readTemperature();
    float humidity = s_dht->readHumidity();

    if (isnan(temp_c) || isnan(humidity)) {
        lv_label_set_text(s_indoor_status_label, "Sensor read failed - check wiring");
        return;
    }

    lv_label_set_text(s_indoor_status_label, "");
    lv_label_set_text_fmt(s_indoor_temp_label, "%.1f C", temp_c);
    lv_label_set_text_fmt(s_indoor_humidity_label, "%.0f %% RH", humidity);
}

void weather_screen_set_web_weather(const WebWeatherData &data) {
    if (!data.valid) {
        lv_label_set_text(s_web_status_label, "Local weather unavailable - check Wi-Fi");
        return;
    }

    lv_label_set_text(s_web_status_label, "");
    lv_label_set_text(s_web_condition_label, web_weather_condition_text(data.weather_code));
    lv_label_set_text_fmt(s_web_temp_label, "%.1f C", data.temp_c);
    lv_label_set_text_fmt(s_web_humidity_label, "%.0f %% RH", data.humidity_pct);
}
