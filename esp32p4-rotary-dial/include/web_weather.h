// Local "today's weather" pulled from the web (Open-Meteo, no API key
// required) over the board's Wi-Fi (routed through the onboard ESP32-C6).
#pragma once

#include <stdbool.h>

struct WebWeatherData {
    bool  valid;
    float temp_c;
    float humidity_pct;
    int   weather_code; // WMO code, see web_weather_condition_text()
};

// Connects to Wi-Fi. Blocks up to ~15s; returns true if connected.
bool web_weather_connect_wifi(const char *ssid, const char *password);

// Fetches the current conditions for WEB_WEATHER_LAT/WEB_WEATHER_LON
// (defined in web_weather.cpp - edit those before flashing). Blocking HTTP
// call; only call this from a low-frequency timer (e.g. every 10-15 min),
// not from the render loop.
WebWeatherData web_weather_fetch(void);

// Short human-readable condition string for an Open-Meteo WMO weather code
// (e.g. "Clear", "Rain", "Snow"). Never returns NULL.
const char *web_weather_condition_text(int weather_code);
