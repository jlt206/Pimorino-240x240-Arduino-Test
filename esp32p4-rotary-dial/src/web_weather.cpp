#include "web_weather.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

// TODO: set this to your actual location before flashing. Look up your
// coordinates (e.g. in any maps app - long-press a point to get lat/lon).
// These are intentionally "null island" so a stale default doesn't silently
// show someone else's weather.
static const double WEB_WEATHER_LAT = 0.0;
static const double WEB_WEATHER_LON = 0.0;

bool web_weather_connect_wifi(const char *ssid, const char *password) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
        delay(250);
    }
    return WiFi.status() == WL_CONNECTED;
}

WebWeatherData web_weather_fetch(void) {
    WebWeatherData result = { false, 0, 0, 0 };

    if (WiFi.status() != WL_CONNECTED) return result;

    char url[192];
    snprintf(url, sizeof(url),
             "https://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f"
             "&current=temperature_2m,relative_humidity_2m,weather_code&timezone=auto",
             WEB_WEATHER_LAT, WEB_WEATHER_LON);

    // Open-Meteo is HTTPS-only. Skipping certificate validation (setInsecure)
    // to avoid having to embed/maintain a root CA bundle - fine for a
    // read-only weather lookup, but worth tightening if you reuse this for
    // anything sensitive.
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    http.begin(client, url);
    int code = http.GET();
    if (code != HTTP_CODE_OK) {
        http.end();
        return result;
    }

    String payload = http.getString();
    http.end();

    JsonDocument doc;
    if (deserializeJson(doc, payload) != DeserializationError::Ok) return result;

    JsonObject current = doc["current"];
    if (current.isNull()) return result;

    result.temp_c       = current["temperature_2m"].as<float>();
    result.humidity_pct = current["relative_humidity_2m"].as<float>();
    result.weather_code = current["weather_code"].as<int>();
    result.valid        = true;
    return result;
}

const char *web_weather_condition_text(int weather_code) {
    // WMO weather interpretation codes, per Open-Meteo's docs.
    switch (weather_code) {
        case 0:  return "Clear";
        case 1:  return "Mainly Clear";
        case 2:  return "Partly Cloudy";
        case 3:  return "Overcast";
        case 45:
        case 48: return "Fog";
        case 51:
        case 53:
        case 55: return "Drizzle";
        case 56:
        case 57: return "Freezing Drizzle";
        case 61:
        case 63:
        case 65: return "Rain";
        case 66:
        case 67: return "Freezing Rain";
        case 71:
        case 73:
        case 75: return "Snow";
        case 77: return "Snow Grains";
        case 80:
        case 81:
        case 82: return "Rain Showers";
        case 85:
        case 86: return "Snow Showers";
        case 95: return "Thunderstorm";
        case 96:
        case 99: return "Thunderstorm w/ Hail";
        default: return "Unknown";
    }
}
