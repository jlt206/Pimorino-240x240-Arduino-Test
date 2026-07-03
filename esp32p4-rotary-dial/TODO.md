# Setup checklist

Everything in this project builds around three unknowns I couldn't verify
from this session: the exact display/touch bring-up for your board, your
network/location details, and which Classic-Bluetooth audio module you'll
pair with it. Work through these in order before flashing.

## 1. PlatformIO / toolchain
- [ ] Install the PlatformIO extension in VS Code, then open the
      `esp32p4-rotary-dial/` folder as a PlatformIO project (not the repo root).
- [ ] First build will download the `pioarduino` platform + all `lib_deps` -
      this takes a while and needs real internet access (large toolchain).
- [ ] If `board = esp32-p4` in `platformio.ini` doesn't resolve, run
      `pio boards | grep -i p4` after the platform installs and swap in
      whatever id it lists.

## 2. Wi-Fi credentials (required for the weather screen's web lookup)
- [ ] Copy `include/secrets.h.example` to `include/secrets.h` (already done
      in this package as a placeholder - just edit the values).
- [ ] Fill in your real `WIFI_SSID` / `WIFI_PASSWORD`.
- [ ] `secrets.h` is gitignored - it won't get committed if you push this
      project to your own repo.

## 3. Weather location
- [ ] In `src/web_weather.cpp`, set `WEB_WEATHER_LAT` / `WEB_WEATHER_LON` to
      your actual coordinates (currently `0.0, 0.0` on purpose, so a forgotten
      default doesn't silently show the wrong place's weather).
- [ ] Uses Open-Meteo (free, no API key). If you'd rather use OpenWeatherMap
      or another provider, that's a straightforward swap in that same file.

## 4. Display + touch bring-up (the biggest unknown)
- [ ] In `src/main.cpp`, replace the `Arduino_GFX *gfx = nullptr;` placeholder
      with a real `Arduino_ESP32DSIPanel` + `Arduino_DSI_Display` construction.
      I couldn't pull Waveshare's exact MIPI-DSI timing parameters for this
      board in this session - get them from Waveshare's demo repo
      (`waveshareteam/ESP32-P4-WIFI6-Touch-LCD-XC`, `examples/arduino/`) or
      their wiki, and copy the working values in.
- [ ] Implement `touch_read_cb()` in `main.cpp` against whatever touch
      controller/library that same demo uses.
- [ ] Confirm `DHT11_PIN` (17) and `BT_UART_RX_PIN`/`BT_UART_TX_PIN` (18/19)
      in `main.cpp` are actually free on your board - cross-check against
      Waveshare's pinout diagram before wiring anything to them.

## 5. Classic-Bluetooth audio module (for real call audio)
- [ ] The board's onboard radio (ESP32-C6) is BLE-only, so actual phone-call
      audio needs a separate Classic-BT/HFP module wired over UART.
- [ ] Once you've picked a module, replace the placeholder AT commands and
      status parsing in `src/bt_audio.cpp` with its real command set.

## 6. Contacts
- [ ] Edit `SPEED_DIAL_CONTACTS` in `include/contacts_screen.h` with your
      real names/numbers.

## 7. First test pass
- [ ] Build + flash (`pio run -t upload`), watch `pio device monitor` at
      115200 baud for Wi-Fi/sensor/BT status lines.
- [ ] Confirm the dial rotates and reports digits over serial even before
      the display is wired up correctly (logic doesn't depend on the panel).
- [ ] Once the panel works: swipe between the three screens, dial a test
      number, check the DHT11 reading, check the local weather panel updates.
