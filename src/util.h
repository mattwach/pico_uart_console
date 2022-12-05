#ifndef UART_CONSOLE_UTIL_H
#define UART_CONSOLE_UTIL_H
// echos a single character
void uart_console_echo(char c);

// prints hex and decimal forms of a character for debugging
void uart_console_debug_echo(char c);
#endif
