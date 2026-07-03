// Speed-dial screen: a short, hand-picked contact list. Tapping a contact
// calls bt_audio_dial() with its number.
#pragma once

#include <lvgl.h>

// Edit this table to your actual contacts.
struct SpeedDialContact {
    const char *name;
    const char *number;
};

// TODO: fill in your real contacts here.
static const SpeedDialContact SPEED_DIAL_CONTACTS[] = {
    { "Home",   "5551234567" },
    { "Mobile", "5559876543" },
    { "Work",   "5555551212" },
};
static const int SPEED_DIAL_CONTACT_COUNT =
    sizeof(SPEED_DIAL_CONTACTS) / sizeof(SPEED_DIAL_CONTACTS[0]);

// Builds the contacts screen as a child of `parent` (an 800x800 tile).
lv_obj_t *contacts_screen_create(lv_obj_t *parent);

// Call periodically (e.g. from a timer) to refresh the connection/call
// status shown at the top of the screen.
void contacts_screen_update_status(void);
