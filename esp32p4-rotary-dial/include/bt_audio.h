// Thin control interface to a separate Classic-Bluetooth hands-free audio
// module (e.g. a BK3266/CSR-based HFP breakout), talked to over UART.
//
// The ESP32-P4's own radio (an onboard ESP32-C6) is BLE-only - it cannot do
// HFP/A2DP, so actual call audio has to be handled by a dedicated
// Classic-BT module wired to a free UART. This header only defines the
// control surface (dial/answer/hangup/status); the exact AT command set
// depends on which module you end up using; see the TODOs in bt_audio.cpp.
#pragma once

#include <stdbool.h>

// Starts the UART link to the audio module. `rx_pin`/`tx_pin` are the
// ESP32-P4 GPIOs wired to the module's TX/RX (cross over: our rx_pin reads
// the module's TX). TODO: confirm both pins are free on your build - avoid
// GPIO7/8 (touch I2C) and anything already claimed by the display.
void bt_audio_init(int rx_pin, int tx_pin, unsigned long baud = 9600);

// Call this often (e.g. once per loop()) to parse incoming status lines
// from the module (connection state, incoming call, etc.).
void bt_audio_poll(void);

// True once the module reports it's paired/connected to a phone.
bool bt_audio_is_connected(void);

// True while the module reports an active or ringing call.
bool bt_audio_is_in_call(void);

// Dials `number` (ASCII digits, no formatting) via the connected phone.
void bt_audio_dial(const char *number);

void bt_audio_answer(void);
void bt_audio_hangup(void);
