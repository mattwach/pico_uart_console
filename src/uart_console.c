#include "uart_console/console.h"
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"

#include "command_history.h"
#include "util.h"
#include "vt102_util.h"

// from vt102_tab_complete.c
void vt102_tab_pressed(struct ConsoleConfig* cc);

// from parse_line.c
void uart_console_parse_line(struct ConsoleConfig* cc);

#define VT102_NORMAL  0x00
#define VT102_ESCAPE  0x01
#define VT102_ESCAPE2 0x02

static void reset_line(struct ConsoleConfig* cc) {
  cc->line_length = 0;
  cc->cursor_index = 0;
  cc->num_args = 0;
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

// Supported operations:
//
// Left arrow
//   027 1b 
//   091 5b [
//   068 44 D
// Right Arrow
//   027 1b 
//   091 5b [
//   067 43 C
// backspace key
//   008 08
// delete key  -- TODO
//   027 1b 
//   091 5b [
//   051 33 3
//   126 7e ~
// ctrl e
//   005 05
// ctrl c
//   003 03
// ctrl a
//   001 01
//
// everything is is ignored
static void vt102_backspace(struct ConsoleConfig* cc) {
  if (cc->cursor_index == 0) {
    // can't backspace
    return;
  }

  if (cc->cursor_index < cc->line_length) {
    // need to deleete a character in the middle of the buffer
    memmove(
      cc->line + cc->cursor_index - 1,
      cc->line + cc->cursor_index,
      cc->line_length - cc->cursor_index);
  }

  --cc->cursor_index;
  --cc->line_length;
  cc->tab_length = cc->line_length;
  vt102_putchar(cc, 0x08); // backspace
  vt102_putchar(cc, 0x1b); // escape
  vt102_putchar(cc, '['); // escape
  vt102_putchar(cc, '1');  // num chars
  vt102_putchar(cc, 'P');  // delete character
}

static char parse_vt102_normal(struct ConsoleConfig* cc, char c) {
  if (c >= 32) {
    // just a regular character
    vt102_putchar(cc, c);
    return c;
  }

  switch (c) {
    case '\r':
      vt102_putchar(cc, c);
      vt102_putchar(cc, '\n');
      return c;
    case '\t':
      vt102_tab_pressed(cc);
      break;
    case 0x1b:
      cc->terminal_state = VT102_ESCAPE;
      break;
    case 0x08:
      vt102_backspace(cc);
      break;
    case 0x05:
      vt102_end_of_line(cc);
      break;
    case 0x01:
      vt102_beginning_of_line(cc);
      break;
    case 0x03:
      return c; // ctrl-c
  }

  return 0;
}

static char parse_vt102_escape2(struct ConsoleConfig* cc, char c) {
  cc->terminal_state = VT102_NORMAL;
  switch (c) {
#if CONSOLE_HISTORY_LINES > 0
    case 'A':
      // up arrow
      vt102_history_previous(cc);
      return 0;
    case 'B':
      // down arrow
      vt102_history_next(cc);
      return 0;
#endif
    case 'D':
      // left arrow
      if (cc->cursor_index == 0) {
        return 0;  // already at zero
      }
      --cc->cursor_index;
      break;
    case 'C':
      // right arrow
      if (cc->cursor_index >= cc->line_length) {
        return 0;  // already at end
      }
      ++cc->cursor_index;
      break;
    default:
      // unsupported sequence
      return 0;
  }

  vt102_putchar(cc, 0x1b);
  vt102_putchar(cc, '[');
  vt102_putchar(cc, c);
  return 0;
}

static char parse_vt102(struct ConsoleConfig* cc, char c) {
  switch (cc->terminal_state) {
    case VT102_NORMAL:
      return parse_vt102_normal(cc, c);
    case VT102_ESCAPE:
      if (c == '[') {
        cc->terminal_state = VT102_ESCAPE2;
        return 0;
      }
      cc->terminal_state = VT102_NORMAL;
      return parse_vt102(cc, c);
    case VT102_ESCAPE2:
      return parse_vt102_escape2(cc, c);
  }

  return c;
}

static void dump_internal_state(struct ConsoleConfig* cc) {
  const char* state = "UNKNOWN";
  switch (cc->terminal_state) {
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

  printf("state=%s line_length=%d cursor_index=%d\n", state, cc->line_length, cc->cursor_index);
  putchar('"');
  for (uint16_t i=0; i<cc->line_length; ++i) {
    putchar(cc->line[i]);
  }
  printf("\"\n");
  putchar(' ');
  for (uint16_t i=0; i<cc->cursor_index; ++i) {
    putchar(' ');
  }
  printf("^\n");
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
      c = parse_vt102(cc, c);
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
    dump_internal_state(cc);
  }
}
