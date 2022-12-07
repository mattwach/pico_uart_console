#include "uart_console/console.h"
#include <string.h>
#include <stdio.h>
#include "pico/stdlib.h"

#include "util.h"
#include "parse_line.h"
#include "vt102_process_char.h"
#include "vt102_util.h"

static void reset_line(struct ConsoleConfig* cc) {
  cc->line_length = 0;
  cc->cursor_index = 0;
  cc->prompt_displayed = 0;
  cc->tab_length = 0;
  cc->tab_callback_index = cc->callback_count - 1;
#if CONSOLE_HISTORY_LINES > 0
  cc->history_marker_index = -1;
#endif
}

void uart_console_init_lowlevel(
  struct ConsoleConfig* cc,
  struct ConsoleCallback* callbacks,
  uint8_t callback_count,
  uint8_t terminal,
  int (*putchar)(int c)) {
  memset(cc, 0, sizeof(struct ConsoleConfig));
  cc->callbacks = callbacks;
  cc->callback_count = callback_count;
  cc->terminal = terminal;
  cc->putchar = putchar;
  reset_line(cc);
}

void uart_console_init(
  struct ConsoleConfig* cc,
  struct ConsoleCallback* callbacks,
  uint8_t callback_count,
  uint8_t terminal) {
  stdio_init_all();
  uart_console_init_lowlevel(
    cc,
    callbacks,
    callback_count,
    terminal,
    putchar);
}


// Processes (and possibly modifies) an incoming character based on the
//console's current mode
static char process_mode(struct ConsoleConfig* cc, char c) {
  switch (cc->terminal) {
    case CONSOLE_MINIMAL:
      // nothing to do
      break;
    case CONSOLE_ECHO:
      console_putchar(cc, c);
      break;
    case CONSOLE_DEBUG_ECHO:
      console_debug_putchar(cc, c);
      break;
    case CONSOLE_VT102:
    case CONSOLE_DEBUG_VT102:
      c = vt102_process_char(cc, c);
      break;
  }
  return c;
}

// inserts a character into cc->line
static void insert_character(struct ConsoleConfig* cc, char c) {
  if (cc->line_length > cc->cursor_index) {
    // need to insert a character into line 
    // make a spot for it
    memmove(
      cc->line + cc->cursor_index + 1,
      cc->line + cc->cursor_index,
      cc->line_length - cc->cursor_index);
  } 

  // fill it in
  cc->line[cc->cursor_index] = c;

  // update indexes
  ++cc->line_length;
  ++cc->cursor_index;
  cc->tab_length = cc->line_length;
} 

// Process a received character from the UART
void uart_console_putchar(struct ConsoleConfig* cc, char c) { 
  c = process_mode(cc, c);
  if (c == '\r') {
    uart_console_parse_line(cc);
    reset_line(cc);
  } else if (c == 0x03) {
    // ctrl-c
    console_puts(cc, "\nCancelled\n");
    reset_line(cc);  
  } else if (c < 32) {
    // ignore this code
  } else if (cc->line_length >= CONSOLE_MAX_LINE_CHARS) {
    console_printf(
        cc, "\nLine too long (>%d characters)\n", CONSOLE_MAX_LINE_CHARS);
    reset_line(cc);
  } else {
    insert_character(cc, c);
  }

  if (cc->terminal == CONSOLE_DEBUG_VT102) {
    vt102_dump_internal_state(cc);
  }
}

// Displays prompt for data
static void show_prompt(struct ConsoleConfig* cc, const char* prompt) {
  console_printf(cc, prompt);
  if (cc->terminal == CONSOLE_VT102) {
    vt102_insert_mode(cc);
  }
  cc->prompt_displayed = 1;
}

uint32_t uart_console_poll(struct ConsoleConfig* cc, const char* prompt) {
  if ((cc->prompt_displayed == 0) && (cc->terminal != CONSOLE_MINIMAL)) {
    show_prompt(cc, prompt);
  }

  uint32_t num_processed = 0;
  while (1) {
    const int cint = getchar_timeout_us(0);
    if ((cint < 0) || (cint > 254)) {
      // didn't get anything
      break;
    }
    uart_console_putchar(cc, (char)cint);
    ++num_processed;
  }

  return num_processed;
}
