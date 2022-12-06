#include "vt102_process_char.h"
#include "vt102_util.h"
#include "command_history.h"
#include <string.h>

#define VT102_NORMAL  0x00
#define VT102_ESCAPE  0x01
#define VT102_ESCAPE2 0x02

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

char vt102_process_char(struct ConsoleConfig* cc, char c) {
  switch (cc->terminal_state) {
    case VT102_NORMAL:
      return parse_vt102_normal(cc, c);
    case VT102_ESCAPE:
      if (c == '[') {
        cc->terminal_state = VT102_ESCAPE2;
        return 0;
      }
      cc->terminal_state = VT102_NORMAL;
      return vt102_process_char(cc, c);
    case VT102_ESCAPE2:
      return parse_vt102_escape2(cc, c);
  }

  return c;
}

void vt102_dump_internal_state(struct ConsoleConfig* cc) {
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

  console_printf(
      cc,
      "state=%s line_length=%d cursor_index=%d\n",
      state,
      cc->line_length,
      cc->cursor_index);
  console_putchar(cc, '"');
  for (uint16_t i=0; i<cc->line_length; ++i) {
    console_putchar(cc, cc->line[i]);
  }
  console_puts(cc, "\"\n ");
  for (uint16_t i=0; i<cc->cursor_index; ++i) {
    console_putchar(cc, ' ');
  }
  console_puts(cc, "^\n");
}

