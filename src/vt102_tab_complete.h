#ifndef VT102_TAB_COMPLETE_H
#define VT102_TAB_COMPLETE_H

// When called, looks though cc->callbacks for a command that
// could complete the current cc->line.
//
// This is one bit of additional complication.  Instead of
// using cc->line_length, cc->tab_length is used instead.
// During normal typing, cc->line_length == cc->tab_length but
// when the user types tab, cc->line_length >= cc->tab_length.
// This is done to allow the user to "scroll through" various
// completion options.
//
// For example, if the defined commands are
//   hello
//   help
//   host
//
// and the use enters "he" and presses tab, the system will cycle
// through "hello" and "help".  If the user just typed "h", then
// the system will cycle through "hello", "help" , and "host".
// cc->tab_length hold the relevant legth (2 in the first example,
// 1 in the second) that implements this behavior.
void vt102_tab_pressed(struct ConsoleConfig* cc);

#endif