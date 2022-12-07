#include "uart_console/console.h"
#include "vt102_util.h"
#include <inttypes.h>
// Functions that handle tab completion

static uint8_t vt102_try_tab_complete(
    struct ConsoleConfig* cc, uint8_t callback_idx) {
  // synthetically adding "help" at the end of the command list
  const char* command = callback_idx >= cc->callback_count ?
    "help" : cc->callbacks[callback_idx].command; 
  uint8_t i=0;
  for (; i < cc->tab_length; ++i) {
    if (cc->line[i] != command[i]) {
      return 0;
    }
  }
  // it's a match
  if (command[i]) {
    uint16_t tab_length = cc->tab_length;
    vt102_replace_current_line(cc, command);
    cc->tab_length = tab_length;
  }
  cc->tab_callback_index = callback_idx;
  return 1;
}

// Called when the user presses the tab key
void vt102_tab_pressed(struct ConsoleConfig* cc) {
  for (uint8_t i=0; i < (cc->callback_count + 1); ++i) {
    const uint8_t callback_idx =
        (cc->tab_callback_index + i + 1) % (cc->callback_count + 1);
    if (vt102_try_tab_complete(cc, callback_idx)) {
      return;
    }
  } 
}
