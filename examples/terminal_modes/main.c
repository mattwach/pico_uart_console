#include "pico/stdlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "uart_console/console.h"

struct ConsoleConfig cc;

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

static void get_terminal(uint8_t argc, char* argv[]) {
  for (uint8_t i=0; i < NUM_TERMINAL_PAIRS; ++i) {
    if (terminal_pairs[i].code == cc.terminal) {
      printf("%s\n", terminal_pairs[i].name);
      return;
    }
  }
  printf("unknown\n");
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

// program entry point
int main(void) {
  uart_console_init(
      &cc,
      callbacks,
      sizeof(callbacks) / sizeof(callbacks[0]),
      CONSOLE_VT102);

  while (1) {
    uart_console_poll(&cc, "> ");
    sleep_ms(20);
  }
  return 0;
}
