#ifndef UART_CONSOLE_PARSE_LINE_H
#define UART_CONSOLE_PARSE_LINE_H
#include "uart_console/console.h"

// This function is called when cc->line is populated and the user issues
// the "enter" key.
//
// The function does the following (delegating some work to other modules):
//  1. Adds the command to command history if CONSOLE_HISTORY_LINES > 0.  This
//     is done even if there is an error so that the user can easily hit
//     up arrow to make corrections.
//  2. Breakup cc->line into arguments by replacing whitespace with null
//     characters
//  3. populates cc->arg[] with string pointers into cc->line
//  4. Looks at the command name to see if there is a matching entry in
//     cc->callbacks
//    a. If not, then print an error with a tip for getting help
//    b. If yes, then validate that the argument count is correct and return an
//       error if not.
//    c. If the arg count is correct, then call the matching cc->callback
void uart_console_parse_line(struct ConsoleConfig* cc);
#endif
