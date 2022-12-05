// Handles logic for command history
#include "command_history.h"
#include "vt102_util.h"
#include <string.h>

#if CONSOLE_HISTORY_LINES > 0
uint8_t maybe_push_line_to_history(struct ConsoleConfig* cc) {
  // first make sure the line is not empty
  uint8_t is_empty = 1;
  for (uint16_t i=0; i<cc->line_length; ++i) {
    if (cc->line[i] != ' ') {
      is_empty = 0;
      break;
    }
  }

  if (is_empty) {
    return 0;
  }

  const char* last_entry = cc->history + (cc->history_tail_index * (CONSOLE_MAX_LINE_CHARS + 1));
  if (!strcmp(cc->line, last_entry)) {
    // repeated command
    return 0;
  }

  cc->history_tail_index = (cc->history_tail_index + 1) % CONSOLE_HISTORY_LINES;
  char* new_entry = cc->history + (cc->history_tail_index * (CONSOLE_MAX_LINE_CHARS + 1));
  memcpy(new_entry, cc->line, cc->line_length);
  new_entry[cc->line_length] = '\0';
  return 1;
}

void vt102_history_previous(struct ConsoleConfig* cc) {
  if (cc->history_marker_index >= (CONSOLE_HISTORY_LINES - 1)) {
    // history is exausted
    return;
  }
  ++cc->history_marker_index;
  const uint16_t slot = (cc->history_tail_index + CONSOLE_HISTORY_LINES - cc->history_marker_index) % CONSOLE_HISTORY_LINES;
  const char* entry = cc->history + (slot * (CONSOLE_MAX_LINE_CHARS + 1));
  if (!entry[0]) {
    // this slot has no data
    --cc->history_marker_index;
    return;
  }

  if (cc->history_marker_index == 0) {
    if (maybe_push_line_to_history(cc)) {
      cc->history_marker_index = 1;
    }
  }
  vt102_replace_current_line(cc, entry);
}

void vt102_history_next(struct ConsoleConfig* cc) {
  if (cc->history_marker_index == -1) {
    // already at the front
    return;
  }

  --cc->history_marker_index;
  if (cc->history_marker_index == -1) {
    vt102_erase_current_line(cc);
    return;
  }
  const uint16_t slot = (cc->history_tail_index + CONSOLE_HISTORY_LINES - cc->history_marker_index) % CONSOLE_HISTORY_LINES;
  const char* entry = cc->history + (slot * (CONSOLE_MAX_LINE_CHARS + 1));
  vt102_replace_current_line(cc, entry);
}
#endif
