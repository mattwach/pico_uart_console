#include "uart_console/console.h"
#include <string.h>
#include <stdio.h>
#include "pico/stdlib.h"

#include "util.h"
#include "parse_line.h"
#include "vt102_process_char.h"

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

void uart_console_init(
  struct ConsoleConfig* cc,
  struct ConsoleCallback* callbacks,
  uint8_t callback_count,
  uint8_t mode) {
  stdio_init_all();
  memset(cc, 0, sizeof(struct ConsoleConfig));
  cc->callbacks = callbacks;
  cc->callback_count = callback_count;
  cc->mode = mode;
  reset_line(cc);
}

void uart_console_poll(
    struct ConsoleConfig* cc,
    const char* prompt) {
  if ((cc->prompt_displayed == 0) && (cc->mode != CONSOLE_MINIMAL)) {
    printf(prompt);
    if (cc->mode == CONSOLE_VT102) {
      // put the terminal in insert mode
      // This is done every line in case the terminal was disconnected or reset.
      putchar(0x1b);
      putchar('[');
      putchar('4');
      putchar('h');
    }
    cc->prompt_displayed = 1;
  }

  const int cint = getchar_timeout_us(0);
  if ((cint < 0) || (cint > 127)) {
    // didn't get anything
    return;
  }

  char c = (char)cint;

  switch (cc->mode) {
    case CONSOLE_MINIMAL:
      // nothing to do
      break;
    case CONSOLE_ECHO:
      uart_console_echo(c);
      break;
    case CONSOLE_DEBUG_ECHO:
      uart_console_debug_echo(c);
      break;
    case CONSOLE_VT102:
    case CONSOLE_DEBUG_VT102:
      c = vt102_process_char(cc, c);
      break;
  }

  if (c == '\r') {
    uart_console_parse_line(cc);
    reset_line(cc);
  } else if (c == 0x03) {
    // ctrl-c
    printf("\nCancelled\n");
    reset_line(cc);  
  } else if (c < 32) {
    // ignore this code
  } else if (cc->line_length >= CONSOLE_MAX_LINE_CHARS) {
    printf("\nLine too long (>%d characters)\n", CONSOLE_MAX_LINE_CHARS);
    reset_line(cc);
  } else {
    if (cc->line_length > cc->cursor_index) {
      // need to insert a charactrer into line 
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

  if (cc->mode == CONSOLE_DEBUG_VT102) {
    vt102_dump_internal_state(cc);
  }
}
