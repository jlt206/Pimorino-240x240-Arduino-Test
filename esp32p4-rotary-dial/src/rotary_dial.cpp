#include "rotary_dial.h"

#include <math.h>

// ---- Layout constants (all in px, for an 800x800 canvas) -----------------
static const lv_coord_t DIAL_SIZE     = 800;
static const lv_coord_t CENTER        = DIAL_SIZE / 2;   // 400
static const lv_coord_t NUMBER_RADIUS = 330;              // digit label ring
static const lv_coord_t LETTER_RADIUS = 372;              // letter label ring
static const lv_coord_t DISC_RADIUS   = 300;              // rotating white disc
static const lv_coord_t DISC_DIAM     = DISC_RADIUS * 2;
static const lv_coord_t HOLE_RING_R   = 225;              // finger-hole ring
static const lv_coord_t HOLE_RADIUS   = 44;
static const lv_coord_t HUB_RADIUS    = 95;               // fixed center hub
static const lv_coord_t STOP_RADIUS   = 300;              // finger-stop ring

// 10 digits + 1 finger-stop gap, evenly spaced around the circle.
static const int   NUM_SLOTS  = 11;
static const float SLOT_ANGLE = 360.0f / NUM_SLOTS; // ~32.727 deg

// Slot order clockwise from 12 o'clock, matching the reference photo.
// -1 marks the fixed finger-stop gap (between "1" and "0", as on a real dial).
static const int8_t SLOT_DIGIT[NUM_SLOTS] = { 3, 2, 1, -1, 0, 9, 8, 7, 6, 5, 4 };

static const char *DIGIT_LETTERS[10] = {
    "OQ",  // 0
    "",    // 1
    "ABC", // 2
    "DEF", // 3
    "GHI", // 4
    "JKL", // 5
    "MN",  // 6
    "PRS", // 7
    "TUV", // 8
    "WXY", // 9
};

static lv_color_t COLOR_BEZEL(void)  { return lv_color_hex(0xEDE0C4); } // cream
static lv_color_t COLOR_DISC(void)   { return lv_color_hex(0xF8F6F1); } // off-white
static lv_color_t COLOR_TEXT(void)   { return lv_color_hex(0x201C14); }
static lv_color_t COLOR_CHROME(void) { return lv_color_hex(0xC9CED3); }
static lv_color_t COLOR_HUB(void)    { return lv_color_hex(0x2B2E33); }
static lv_color_t COLOR_HUB_TX(void) { return lv_color_hex(0xE8E8E8); }

// ---- State -----------------------------------------------------------
static lv_obj_t *s_disc = NULL; // the rotating object (holes are its children)

static float s_rotation_deg    = 0;   // current clockwise rotation of the disc
static int   s_grabbed_digit   = -1;  // digit held during a drag, -1 = none
static float s_touch_start_deg = 0;
static float s_rotation_start  = 0;

static rotary_dial_digit_cb_t s_cb = NULL;

// ---- Small helpers -----------------------------------------------------

// Screen-space angle helper: 0 deg = 12 o'clock, clockwise positive.
static void polar_to_xy(float angle_deg, float r, lv_coord_t *x, lv_coord_t *y) {
    float rad = angle_deg * (float)M_PI / 180.0f;
    *x = CENTER + (lv_coord_t)lroundf(r * sinf(rad));
    *y = CENTER - (lv_coord_t)lroundf(r * cosf(rad));
}

static float norm_angle(float deg) {
    while (deg < 0) deg += 360.0f;
    while (deg >= 360.0f) deg -= 360.0f;
    return deg;
}

// Shortest signed difference a-b, result in (-180, 180].
static float angle_diff(float a, float b) {
    float d = norm_angle(a - b);
    if (d > 180.0f) d -= 360.0f;
    return d;
}

static int slot_for_digit(int digit) {
    for (int k = 0; k < NUM_SLOTS; k++)
        if (SLOT_DIGIT[k] == digit) return k;
    return -1;
}

static int stop_slot(void) {
    for (int k = 0; k < NUM_SLOTS; k++)
        if (SLOT_DIGIT[k] == -1) return k;
    return -1;
}

// Clockwise travel (degrees) from a digit's rest position to the finger
// stop. This naturally works out to the real telephone pulse count (1-9,
// and 10 for "0") given the slot order above.
static float max_travel_deg(int digit) {
    int k_d = slot_for_digit(digit);
    int k_s = stop_slot();
    int slots = ((k_s - k_d) + NUM_SLOTS) % NUM_SLOTS;
    return slots * SLOT_ANGLE;
}

static void place_label_centered(lv_obj_t *label, lv_coord_t cx, lv_coord_t cy) {
    lv_obj_update_layout(label);
    lv_coord_t w = lv_obj_get_width(label);
    lv_coord_t h = lv_obj_get_height(label);
    lv_obj_set_pos(label, cx - w / 2, cy - h / 2);
}

static void set_disc_rotation(float deg) {
    s_rotation_deg = deg;
    // v8 equivalent: lv_obj_set_style_transform_angle(). v9 renamed the
    // style property to "rotation" (still in 0.1-degree units).
    lv_obj_set_style_transform_rotation(s_disc, (int16_t)lroundf(deg * 10.0f), 0);
}

static lv_obj_t *make_flat_circle(lv_obj_t *parent, lv_coord_t diam, lv_color_t bg,
                                   lv_color_t border, lv_coord_t border_w) {
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_set_size(o, diam, diam);
    lv_obj_set_style_radius(o, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(o, bg, 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(o, border_w, 0);
    lv_obj_set_style_border_color(o, border, 0);
    lv_obj_set_style_border_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_shadow_width(o, 0, 0);
    lv_obj_set_style_pad_all(o, 0, 0);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    return o;
}

// ---- The rotating disc: a plain widget whose children (the holes) rotate
// with it, via the generic per-object transform style properties. --------
static void create_disc(lv_obj_t *root) {
    s_disc = make_flat_circle(root, DISC_DIAM, COLOR_DISC(), lv_color_darken(COLOR_DISC(), 40), 3);
    lv_obj_set_pos(s_disc, CENTER - DISC_RADIUS, CENTER - DISC_RADIUS);
    lv_obj_set_style_transform_pivot_x(s_disc, DISC_RADIUS, 0);
    lv_obj_set_style_transform_pivot_y(s_disc, DISC_RADIUS, 0);

    for (int k = 0; k < NUM_SLOTS; k++) {
        if (SLOT_DIGIT[k] < 0) continue; // finger-stop gap: no hole
        float angle = k * SLOT_ANGLE;
        lv_coord_t hx, hy; // screen-space position at rest
        polar_to_xy(angle, HOLE_RING_R, &hx, &hy);

        lv_obj_t *hole = make_flat_circle(s_disc, HOLE_RADIUS * 2, COLOR_BEZEL(),
                                           lv_color_darken(COLOR_BEZEL(), 30), 2);
        // Position relative to the disc's own local box (disc-local origin
        // is its top-left corner; disc-local center is DISC_RADIUS,DISC_RADIUS).
        lv_coord_t lx = DISC_RADIUS + (hx - CENTER) - HOLE_RADIUS;
        lv_coord_t ly = DISC_RADIUS + (hy - CENTER) - HOLE_RADIUS;
        lv_obj_set_pos(hole, lx, ly);
    }
}

// ---- Touch handling ------------------------------------------------------
static void spring_back_anim_cb(void *var, int32_t v) {
    (void)var;
    set_disc_rotation((float)v / 10.0f);
}

static void spring_back_ready_cb(lv_anim_t *a) {
    (void)a;
    if (s_grabbed_digit >= 0 && s_cb) {
        char digit = (char)('0' + s_grabbed_digit);
        s_cb(digit);
    }
    s_grabbed_digit = -1;
}

static void start_spring_back(void) {
    int digit = s_grabbed_digit; // captured before it's cleared on anim end
    float travel = max_travel_deg(digit);
    // ~70ms per pulse-slot, like the real mechanical governor.
    uint32_t duration = (uint32_t)lroundf((travel / SLOT_ANGLE) * 70.0f);
    if (duration < 50) duration = 50;

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, NULL);
    lv_anim_set_values(&a, (int32_t)lroundf(s_rotation_deg * 10.0f), 0);
    lv_anim_set_time(&a, duration);
    lv_anim_set_exec_cb(&a, spring_back_anim_cb);
    lv_anim_set_ready_cb(&a, spring_back_ready_cb);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in);
    lv_anim_start(&a);
}

static void dial_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t *indev = lv_indev_get_act();
    if (!indev) return;

    lv_point_t p;
    lv_indev_get_point(indev, &p);
    float dx = (float)(p.x - CENTER);
    float dy = (float)(p.y - CENTER);
    float touch_deg = norm_angle(atan2f(dx, -dy) * 180.0f / (float)M_PI);
    float radius = sqrtf(dx * dx + dy * dy);

    if (code == LV_EVENT_PRESSED) {
        s_grabbed_digit = -1;
        if (radius < HOLE_RING_R - HOLE_RADIUS * 1.5f || radius > HOLE_RING_R + HOLE_RADIUS * 1.5f)
            return;

        // Which hole (at rest) is under the finger right now, given the
        // disc's current rotation?
        float rest_deg = norm_angle(touch_deg - s_rotation_deg);
        int best_slot = -1;
        float best_diff = 1e9f;
        for (int k = 0; k < NUM_SLOTS; k++) {
            if (SLOT_DIGIT[k] < 0) continue;
            float diff = fabsf(angle_diff(rest_deg, k * SLOT_ANGLE));
            if (diff < best_diff) { best_diff = diff; best_slot = k; }
        }
        if (best_slot < 0 || best_diff > SLOT_ANGLE * 0.4f) return;

        s_grabbed_digit    = SLOT_DIGIT[best_slot];
        s_touch_start_deg  = touch_deg;
        s_rotation_start   = s_rotation_deg;
    } else if (code == LV_EVENT_PRESSING) {
        if (s_grabbed_digit < 0) return;
        float delta = angle_diff(touch_deg, s_touch_start_deg);
        float new_rotation = s_rotation_start + delta;
        float max_deg = max_travel_deg(s_grabbed_digit);
        if (new_rotation < 0) new_rotation = 0;
        if (new_rotation > max_deg) new_rotation = max_deg;
        set_disc_rotation(new_rotation);
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        if (s_grabbed_digit >= 0) start_spring_back();
    }
}

// ---- Fixed faceplate: number/letter ring + finger stop + hub -------------
static void create_faceplate(lv_obj_t *root) {
    for (int k = 0; k < NUM_SLOTS; k++) {
        int digit = SLOT_DIGIT[k];
        if (digit < 0) continue;
        float angle = k * SLOT_ANGLE;

        lv_obj_t *num = lv_label_create(root);
        lv_obj_set_style_text_font(num, &lv_font_montserrat_28, 0);
        lv_obj_set_style_text_color(num, COLOR_TEXT(), 0);
        lv_label_set_text_fmt(num, "%d", digit);
        lv_coord_t nx, ny;
        polar_to_xy(angle, NUMBER_RADIUS, &nx, &ny);
        place_label_centered(num, nx, ny);

        if (DIGIT_LETTERS[digit][0] != '\0') {
            lv_obj_t *letters = lv_label_create(root);
            lv_obj_set_style_text_font(letters, &lv_font_montserrat_16, 0);
            lv_obj_set_style_text_color(letters, COLOR_TEXT(), 0);
            lv_label_set_text(letters, DIGIT_LETTERS[digit]);
            lv_coord_t lx, ly;
            polar_to_xy(angle, LETTER_RADIUS, &lx, &ly);
            place_label_centered(letters, lx, ly);
        }
    }

    // Fixed chrome finger stop, sitting at the gap slot, drawn after (i.e.
    // in front of) the disc.
    int k_stop = stop_slot();
    lv_obj_t *stop = lv_obj_create(root);
    lv_obj_clear_flag(stop, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(stop, 34, 70);
    lv_obj_set_style_radius(stop, 8, 0);
    lv_obj_set_style_bg_color(stop, COLOR_CHROME(), 0);
    lv_obj_set_style_border_width(stop, 2, 0);
    lv_obj_set_style_border_color(stop, lv_color_darken(COLOR_CHROME(), 40), 0);
    lv_obj_set_style_shadow_width(stop, 0, 0);
    lv_obj_set_style_transform_pivot_x(stop, 17, 0);
    lv_obj_set_style_transform_pivot_y(stop, 35, 0);
    lv_obj_set_style_transform_rotation(stop, (int16_t)lroundf(k_stop * SLOT_ANGLE * 10.0f), 0);
    lv_coord_t sx, sy;
    polar_to_xy(k_stop * SLOT_ANGLE, STOP_RADIUS, &sx, &sy);
    lv_obj_set_pos(stop, sx - 17, sy - 35);

    // Fixed center hub, drawn last so it sits above the rotating disc.
    lv_obj_t *hub = make_flat_circle(root, HUB_RADIUS * 2, COLOR_HUB(), lv_color_darken(COLOR_CHROME(), 10), 3);
    lv_obj_set_pos(hub, CENTER - HUB_RADIUS, CENTER - HUB_RADIUS);

    lv_obj_t *hub_label = lv_label_create(hub);
    lv_obj_set_style_text_font(hub_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(hub_label, COLOR_HUB_TX(), 0);
    lv_obj_set_style_text_align(hub_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(hub_label, "DIAL\n999");
    lv_obj_center(hub_label);
}

// ---- Public API -----------------------------------------------------------
lv_obj_t *rotary_dial_create(lv_obj_t *parent) {
    lv_obj_t *root = lv_obj_create(parent);
    lv_obj_set_size(root, DIAL_SIZE, DIAL_SIZE);
    lv_obj_set_style_radius(root, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(root, COLOR_BEZEL(), 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_shadow_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 0, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(root);

    // Disc created before the faceplate's finger-stop/hub so those stack
    // visually on top; the number/letter labels sit further out and never
    // overlap the disc.
    create_disc(root);
    set_disc_rotation(0);
    create_faceplate(root);

    // All touch handling happens on `root`; the disc, holes, stop and hub
    // are all non-clickable so hits fall through to it uniformly.
    lv_obj_add_event_cb(root, dial_event_cb, LV_EVENT_ALL, NULL);

    return root;
}

void rotary_dial_set_callback(rotary_dial_digit_cb_t cb) {
    s_cb = cb;
}
