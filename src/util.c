#include "uart_console/console.h"
#include "util.h"
#include <stdarg.h>
#include <stdio.h>

void console_putchar(const struct ConsoleConfig* cc, char c) {
  if (c == '\r') {
    cc->putchar('\r');
    cc->putchar('\n');  // also one of these
  } else if (c >= 32) {
    cc->putchar(c);
  }
}

void console_puts(const struct ConsoleConfig* cc, const char* s) {
  for (; *s; ++s) {
    console_putchar(cc, *s);
  }
}

void console_debug_putchar(const struct ConsoleConfig* cc, char c) {
  console_printf(cc, "%03d %02x ", c, c);
  if ((c >= 32) && (c <= 126)) {
    cc->putchar(c);
  }
  console_printf(cc, "\n");
}

#define MAX_PRINTF_LENGTH 80
void console_printf(const struct ConsoleConfig* cc, const char* fmt, ...) {
  static char printf_buffer[MAX_PRINTF_LENGTH + 1];
  va_list args;
  va_start(args, fmt);
  vsnprintf(printf_buffer, MAX_PRINTF_LENGTH, fmt, args);
  va_end(args);
  for (const char* c=printf_buffer; *c; ++c) {
    cc->putchar(*c);
  }
}

