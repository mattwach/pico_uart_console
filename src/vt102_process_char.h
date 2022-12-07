#ifndef VT102_PROCESS_CHAR_H
#define VT102_PROCESS_CHAR_H

#include "uart_console/console.h"
#include "vt102_tab_complete.h"

// Handles processing of VT102 escape sequences to support a basic
// "readline" editing environment.
//
// Support is not comprehensive.  For example, Ctrl-W is not implemented
// (but could be added as needed). Currently-supported operations:
//
// Up Arrow - recall previous history - only available if CONSOLE_HISTORY_LINES > 0
//   027 1b ESC
//   091 5b [
//   067 43 A
// Down Arrow - go forward in history - only available if CONSOLE_HISTORY_LINES > 0
//   027 1b ESC
//   091 5b [
//   067 43 B
// Right Arrow - move cursor forward one space
//   027 1b ESC
//   091 5b [
//   067 43 C
// Left arrow - move cursor back one space
//   027 1b ESC
//   091 5b [
//   068 44 D
// backspace key - delete character to the left of the cursor
//   008 08
// delete key  -- TODO
//   027 1b ESC 
//   091 5b [
//   051 33 3
//   126 7e ~
// ctrl e - move cursor to the end of the current line
//   005 05
// ctrl c - cancel the current line
//   003 03
// ctrl a - move cursor to the beginning of the current line
//   001 01
// ASCII ' ' through '~': Add a character
// 
// Assuming this comment is not outdated (check the code), all other control
// and escape sequences are absorbed and ignored.
char vt102_process_char(struct ConsoleConfig* cc, char c);

// Used in CONSOLE_DEBUG_VT102 mode to dump the internal state.
// This a useful mode when adding support for new features or
// investigating unexpected interactions between the host machine
// and pico (which are expected to both follow the same escape control
// protcol but that doesn't always work and is affected by thing like
// the TERM environment variable on the host side).
void vt102_dump_internal_state(struct ConsoleConfig* cc);
#endif