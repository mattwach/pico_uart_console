#ifndef UART_CONSOLE_UTIL_H
#define UART_CONSOLE_UTIL_H
// echos a single character
void console_putchar(const struct ConsoleConfig* cc, char c);

// output a string
void console_puts(const struct ConsoleConfig* cc, const char* s);

// prints hex and decimal forms of a character for debugging
void console_debug_putchar(const struct ConsoleConfig* cc, char c);

// prints a formatted string (of limited size)
void console_printf(const struct ConsoleConfig* cc, const char* fmt, ...);
#endif
