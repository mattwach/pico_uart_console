#include "util.h"
#include <stdio.h>

void uart_console_echo(char c) {
  if (c == '\r') {
    putchar('\r');
    putchar('\n');  // also one of these
  } else if (c >= 32) {
    putchar(c);
  }
}

void uart_console_debug_echo(char c) {
  printf("%03d %02x ", c, c);
  if ((c >= 32) && (c <= 126)) {
    putchar(c);
  }
  printf("\n");
}
