
#include "vt102_util.h"
#include <string.h>

static void vt102_put_ascii_number(struct ConsoleConfig* cc, uint16_t delta) {
  if (delta == 0) {
    // nothing
    return;
  }
  char digits[8];
  uint8_t length = 0;
  for (; delta; ++length, delta /= 10) {
    digits[length] = '0' + (delta % 10);
  }
  for (; length > 0; --length) {
    vt102_putchar(cc, digits[length - 1]);
  }
}

void vt102_beginning_of_line(struct ConsoleConfig* cc) {
  if (cc->cursor_index == 0) {
    // already at beginning
    return;
  }
  vt102_putchar(cc, 0x1b); // escape
  vt102_putchar(cc, '['); // escape
  vt102_put_ascii_number(cc, cc->cursor_index);
  vt102_putchar(cc, 'D');  // cursor left
  cc->cursor_index = 0;
}

void vt102_end_of_line(struct ConsoleConfig* cc) {
  if (cc->cursor_index >= cc->line_length) {
    // already at eol
    return;
  }
  vt102_putchar(cc, 0x1b); // escape
  vt102_putchar(cc, '['); // escape
  vt102_put_ascii_number(cc, cc->line_length - cc->cursor_index);
  vt102_putchar(cc, 'C');  // cursor right
  cc->cursor_index = cc->line_length;
}

void vt102_erase_current_line(struct ConsoleConfig* cc) {
  if (cc->line_length == 0) {
    // already empty
    return;
  }
  vt102_beginning_of_line(cc);
  vt102_putchar(cc, 0x1b); // escape
  vt102_putchar(cc, '['); // escape
  vt102_put_ascii_number(cc, cc->line_length);
  vt102_putchar(cc, 'P');  // delete character

  cc->line_length = 0;
  cc->line[0] = 0;
}

void vt102_replace_current_line(struct ConsoleConfig* cc, const char* line) {
  vt102_erase_current_line(cc);
  const uint16_t new_length = strlen(line);
  memcpy(cc->line, line, new_length);
  cc->line_length = new_length;
  cc->cursor_index = new_length;
  cc->tab_length = cc->line_length;
  for (uint16_t i=0; i<new_length; ++i) {
    vt102_putchar(cc, line[i]);
  }
}
