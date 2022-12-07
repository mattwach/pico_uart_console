#include "pico/stdlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sharpdisp/sharpconsole.h"
#include "fonts/liberation_mono_14.h"
#include "uart_console/console.h"

#define WIDTH 400
#define HEIGHT 240

struct ConsoleConfig cc;
uint8_t disp_buffer[BITMAP_SIZE(WIDTH, HEIGHT)];
struct SharpDisp sd;
struct Console console;  // for sharpdisp
uint32_t report_index;

struct TerminalPair {
  const char* name;
  uint8_t code;
};

struct TerminalPair terminal_pairs[] = {
  {"debug", CONSOLE_DEBUG_ECHO},
  {"debug_vt102", CONSOLE_DEBUG_VT102},
  {"echo", CONSOLE_ECHO},
  {"minimal", CONSOLE_MINIMAL},
  {"vt102", CONSOLE_VT102},
};
#define NUM_TERMINAL_PAIRS (sizeof(terminal_pairs) / sizeof(terminal_pairs[0]))

static void list_terminals(uint8_t argc, char* argv[]) {
  for (uint8_t i=0; i < NUM_TERMINAL_PAIRS; ++i) {
    printf("  %s\n", terminal_pairs[i].name);
  }
}

static void set_terminal(uint8_t argc, char* argv[]) {
  for (uint8_t i=0; i < NUM_TERMINAL_PAIRS; ++i) {
    if (!strcmp(argv[0], terminal_pairs[i].name)) {
      cc.terminal = terminal_pairs[i].code;
      return;
    }
  }
  printf("Unknown terminal.  Try list_terminals\n");
}

static const char* get_terminal_name() {
  for (uint8_t i=0; i < NUM_TERMINAL_PAIRS; ++i) {
    if (terminal_pairs[i].code == cc.terminal) {
      return terminal_pairs[i].name;
    }
  }
  return "unknown";
}

static void get_terminal(uint8_t argc, char* argv[]) {
  printf("%s\n", get_terminal_name());
}

static void hello(uint8_t argc, char* argv[]) {
  printf("Hello World!\n");
}

// Configuration to register with uart_console_init()
struct ConsoleCallback callbacks[] = {
    {"hello", "Welcome message", 0, hello},
    {"get_terminal", "Gets terminal", 0, get_terminal},
    {"list_terminals", "List known terminals", 0, list_terminals},
    {"set_terminal", "Sets terminal", 1, set_terminal},
};

static void sharp_debug_init(void) {
  sleep_ms(100);  // allow voltage to stabilize
  sharpdisp_init(
      &sd,
      disp_buffer,
      WIDTH,
      HEIGHT,
      0x00,
      1,  // GP1 for CS (Adafruit RP2040)
      18,  // GP18 for SCK
      19,  // GP19 for MOSI
      spi0,
      10000000
  );
  sharpconsole_init(&console, &sd, liberation_mono_14, 32);
  sharpconsole_printf(&console, "Console Initialized\n");
  sharpconsole_flush(&console);
}

static void sharp_debug_dump() {
  sharpconsole_clear(&console);
  sharpconsole_printf(&console, "idx: %d terminal: %s\n", report_index++, get_terminal_name());
  const char* state = "UNKNOWN";
  switch (cc.terminal_state) {
    case VT102_NORMAL:
      state = "NORMAL";
      break;
    case VT102_ESCAPE:
      state = "ESCAPE";
      break;
    case VT102_ESCAPE2:
      state = "ESCAPE2";
      break;
  }

  sharpconsole_printf(&console, "state=%s line=%d cursor=%d\n", state, cc.line_length, cc.cursor_index);
  sharpconsole_printf(&console, "tab_len=%d tab_cb_idx=%d\n", cc.tab_length, cc.tab_callback_index);
  sharpconsole_printf(&console, "line: |");
  for (uint16_t i=0; i<cc.line_length; ++i) {
    sharpconsole_char(&console, cc.line[i]);
  }
  sharpconsole_printf(&console, "|\n");
  sharpconsole_printf(&console, "curs:  ");
  for (uint16_t i=0; i<cc.cursor_index; ++i) {
    sharpconsole_char(&console, ' ');
  }
  sharpconsole_printf(&console, "^\n\n");

  const uint16_t marker_idx = (cc.history_tail_index + CONSOLE_HISTORY_LINES - cc.history_marker_index) % CONSOLE_HISTORY_LINES;
  for (uint16_t i=0; i<CONSOLE_HISTORY_LINES; ++i) {
    const char is_tail = (i == cc.history_tail_index) ? 'T' : ' ';
    const char is_marker = (i == marker_idx) ? 'M' : ' ';
    const char* line = cc.history + (i * (CONSOLE_MAX_LINE_CHARS + 1));
    sharpconsole_printf(
      &console,
      "  %c%c | %s\n", is_marker, is_tail, line);
  }

  sharpconsole_flush(&console);
}

// program entry point
int main() {
  sleep_ms(500); // let sharp display power up
  uart_console_init(
      &cc,
      callbacks,
      sizeof(callbacks) / sizeof(callbacks[0]),
      CONSOLE_VT102);

  sharp_debug_init();

  while (1) {
    if (uart_console_poll(&cc, "$ ")) {
      sharp_debug_dump();
    }
    sleep_ms(20);
  }
  return 0;
}
