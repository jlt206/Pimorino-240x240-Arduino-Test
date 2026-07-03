// Temperature/humidity screen backed by a DHT11 sensor.
#pragma once

#include <lvgl.h>

// Builds the weather screen as a child of `parent` (an 800x800 tile).
// `dht_pin` is the GPIO the DHT11 data line is wired to.
lv_obj_t *weather_screen_create(lv_obj_t *parent, int dht_pin);

// Reads the sensor and refreshes the on-screen readout. Call this no more
// often than every ~2s - that's the DHT11's own sampling limit.
void weather_screen_update(void);
