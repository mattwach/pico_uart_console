#ifndef CONSOLE_H
#define CONSOLE_H

#include <inttypes.h>

#define CONSOLE_MAX_LINE_CHARS 80
#define CONSOLE_MAX_ARGS 16
#define CONSOLE_HISTORY_LINES 10  // set to zero to disable

// Console Mode
// Consumes characters 32-126.  No echo or editing.
#define CONSOLE_MINIMAL          0x00
// Consumes characters 32-126.  Echos consumed characters.  No editing.
#define CONSOLE_ECHO             0x01
// Consumes characters 32-126.  Echos all characters back as codes.  No editing.
#define CONSOLE_DEBUG_ECHO       0x03
// Tries to emulate VT102 at a basic level.  Supports ctrl-a, ctrl-c, ctrl-e,
// del, backspace, and arrows
#define CONSOLE_VT102            0x04
// VT102 debug mode.  Instead of echoning back codes, it shows internal state
#define CONSOLE_DEBUG_VT102      0x05

struct ConsoleCallback {
  const char* command;
  const char* description;
  void (*callback)(uint8_t argc, char* argv[]);
};

struct ConsoleConfig {
  // Configuration
  struct ConsoleCallback* callbacks;
  uint8_t callback_count;

  // Basic state that applies to all modes of operation
  char line[CONSOLE_MAX_LINE_CHARS + 1];
  uint16_t line_length;
  char* arg[CONSOLE_MAX_ARGS];
  uint8_t num_args;
  uint8_t mode;
  uint8_t prompt_displayed;

  // extra state needed for vt102 modes
  uint8_t terminal_state;  // vt102 state tracking
  uint16_t cursor_index;  // used with vt102

#if CONSOLE_HISTORY_LINES > 0
  // state needed for history support
  char history[(CONSOLE_MAX_LINE_CHARS + 1) * CONSOLE_HISTORY_LINES];
  // contains the tail index (index of last entry written)
  uint16_t history_tail_index;
  // constains the number of entries to look backwards in the queue (usually zero)
  uint16_t history_marker_index;
#endif
};

// Initializes console
void console_init(
  struct ConsoleConfig* cc,
  struct ConsoleCallback* callbacks,
  uint8_t callback_count,
  uint8_t flags);

// Polls for some characters.  This function may call any of the callbacks
// defined in ConsoleConfig before returning.
void console_poll(struct ConsoleConfig* cc, const char* prompt);

#endif
