#ifndef UART_COMMAND_HISTORY_H
#define UART_COMMAND_HISTORY_H
#include "uart_console/console.h"

// Pushes a line to command history if not empty and not identical
// to the last-entered line.
uint8_t maybe_push_line_to_history(struct ConsoleConfig* cc);

// looks at previous history (up arrow)
void vt102_history_previous(struct ConsoleConfig* cc);

// looks and next history (down arrow)
void vt102_history_next(struct ConsoleConfig* cc);
#endif
