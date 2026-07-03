#include "contacts_screen.h"

#include "bt_audio.h"

static lv_obj_t *s_status_label = nullptr;

static void contact_btn_event_cb(lv_event_t *e) {
    const char *number = (const char *)lv_event_get_user_data(e);
    bt_audio_dial(number);
}

lv_obj_t *contacts_screen_create(lv_obj_t *parent) {
    lv_obj_t *root = lv_obj_create(parent);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(root, lv_color_hex(0x1A1A1A), 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 40, 0);
    lv_obj_set_style_pad_row(root, 20, 0);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(root, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *title = lv_label_create(root);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_label_set_text(title, "Speed Dial");

    s_status_label = lv_label_create(root);
    lv_obj_set_style_text_font(s_status_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_status_label, lv_color_hex(0xAAAAAA), 0);
    lv_label_set_text(s_status_label, "Phone: disconnected");

    for (int i = 0; i < SPEED_DIAL_CONTACT_COUNT; i++) {
        lv_obj_t *btn = lv_button_create(root);
        lv_obj_set_size(btn, 400, 90);
        lv_obj_set_style_radius(btn, 20, 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x2E7D32), 0);
        lv_obj_add_event_cb(btn, contact_btn_event_cb, LV_EVENT_CLICKED,
                             (void *)SPEED_DIAL_CONTACTS[i].number);

        lv_obj_t *label = lv_label_create(btn);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_28, 0);
        lv_obj_set_style_text_color(label, lv_color_white(), 0);
        lv_label_set_text(label, SPEED_DIAL_CONTACTS[i].name);
        lv_obj_center(label);
    }

    return root;
}

void contacts_screen_update_status(void) {
    if (!s_status_label) return;
    if (bt_audio_is_in_call()) {
        lv_label_set_text(s_status_label, "Phone: in call");
    } else if (bt_audio_is_connected()) {
        lv_label_set_text(s_status_label, "Phone: connected");
    } else {
        lv_label_set_text(s_status_label, "Phone: disconnected");
    }
}
