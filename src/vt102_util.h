#ifndef UART_CONSOLE_VT102_UTIL_H
#define UART_CONSOLE_VT102_UTIL_H
// Defines common utility functions that are needed by more than one module
#include "uart_console/console.h"
#include "util.h"
#include <inttypes.h>

// outputs a single character
static inline void vt102_putchar(struct ConsoleConfig* cc, char c) {
  if (cc->terminal == CONSOLE_DEBUG_VT102) {
    console_debug_putchar(cc, c);
  } else {
    cc->putchar(c);
  }
}

// moves the cursor to the beginning of a line
void vt102_beginning_of_line(struct ConsoleConfig* cc);

// moves the cursor to the end of a line
void vt102_end_of_line(struct ConsoleConfig* cc);

// Completely erases the current line
void vt102_erase_current_line(struct ConsoleConfig* cc);

// Replaces the current line with the given string
void vt102_replace_current_line(struct ConsoleConfig* cc, const char* line);

// Instructs the teminal to use insert mode
void vt102_insert_mode(struct ConsoleConfig* cc);
#endif
