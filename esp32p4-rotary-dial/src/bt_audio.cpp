#include "bt_audio.h"

#include <Arduino.h>
#include <string.h>

// TODO: this whole file assumes a generic "send AT command, watch for
// unsolicited status lines" module. Replace the command strings/parsing
// below with whatever your specific Classic-BT HFP module actually speaks -
// they vary a lot between vendors (BK3266/CSR/etc. modules all differ).
// The public API in bt_audio.h is meant to stay stable even if this
// implementation changes completely once you've picked a module.

static HardwareSerial *s_uart = nullptr;
static bool s_connected = false;
static bool s_in_call   = false;
static char s_line_buf[64];
static size_t s_line_len = 0;

void bt_audio_init(int rx_pin, int tx_pin, unsigned long baud) {
    static HardwareSerial uart(1); // UART1; swap the port number if it conflicts with something else
    s_uart = &uart;
    s_uart->begin(baud, SERIAL_8N1, rx_pin, tx_pin);
}

static void handle_line(const char *line) {
    // TODO: replace with your module's real unsolicited-status format.
    if (strstr(line, "CONNECTED"))    s_connected = true;
    if (strstr(line, "DISCONNECTED")) { s_connected = false; s_in_call = false; }
    if (strstr(line, "RING") || strstr(line, "CALL=1")) s_in_call = true;
    if (strstr(line, "CALL=0") || strstr(line, "NO CARRIER")) s_in_call = false;
}

void bt_audio_poll(void) {
    if (!s_uart) return;
    while (s_uart->available()) {
        char c = (char)s_uart->read();
        if (c == '\n' || c == '\r') {
            if (s_line_len > 0) {
                s_line_buf[s_line_len] = '\0';
                handle_line(s_line_buf);
                s_line_len = 0;
            }
        } else if (s_line_len < sizeof(s_line_buf) - 1) {
            s_line_buf[s_line_len++] = c;
        }
    }
}

bool bt_audio_is_connected(void) { return s_connected; }
bool bt_audio_is_in_call(void)   { return s_in_call; }

void bt_audio_dial(const char *number) {
    if (!s_uart) return;
    // TODO: real dial command, e.g. "AT+DIAL=<number>\r\n" - confirm syntax
    // against your module's datasheet.
    s_uart->printf("AT+DIAL=%s\r\n", number);
}

void bt_audio_answer(void) {
    if (!s_uart) return;
    s_uart->print("AT+ANSWER\r\n"); // TODO: confirm real command
}

void bt_audio_hangup(void) {
    if (!s_uart) return;
    s_uart->print("AT+HANGUP\r\n"); // TODO: confirm real command
}
