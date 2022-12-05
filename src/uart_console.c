#include "uart_console/console.h"
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"

#define VT102_NORMAL  0x00
#define VT102_ESCAPE  0x01
#define VT102_ESCAPE2 0x02

static void reset_line(struct ConsoleConfig* cc) {
  cc->line_length = 0;
  cc->cursor_index = 0;
  cc->num_args = 0;
  cc->prompt_displayed = 0;
  cc->tab_length = 0;
  cc->tab_callback_index = cc->callback_count - 1;
#if CONSOLE_HISTORY_LINES > 0
  cc->history_marker_index = -1;
#endif
}

void uart_console_init(
  struct ConsoleConfig* cc,
  struct ConsoleCallback* callbacks,
  uint8_t callback_count,
  uint8_t mode) {
  stdio_init_all();
  memset(cc, 0, sizeof(struct ConsoleConfig));
  cc->callbacks = callbacks;
  cc->callback_count = callback_count;
  cc->mode = mode;
  reset_line(cc);
}

static int convert_spaces_to_nulls(char* line, uint16_t line_length) {
  int16_t quote_start = -1;
  uint8_t backslash = 0;
  uint8_t last_was_whitespace = 1;
  for (uint16_t i=0; i<line_length; ++i) {
    const char c = line[i];
    if (backslash) {
      last_was_whitespace = 0;
      backslash = 0;
    } else if (c == '\\') {
      last_was_whitespace = 0;
      backslash = 1;
    } else if (quote_start >= 0) {
      last_was_whitespace = 0;
      if (c == '"') {
        if ((i - quote_start) == 1) {
          // special case empty string
          line[i] = 0xFF;
        } else {
          line[i] = 0;
        }
        line[quote_start] = 0;
        quote_start = -1;
      }
    } else if ((c == '"') && last_was_whitespace) {
      line[i] = 0;
      quote_start = i;
      last_was_whitespace = 0;
    } else if (c == ' ') {
      last_was_whitespace = 1;
      line[i] = 0;
    } else {
      last_was_whitespace = 0;
    }
  }

  if (quote_start >= 0) {
    printf("Unclosed quote\n");
    return 0;
  }
  if (backslash) {
    printf("Line ended with backslash\n");
    return 0;
  }
  return 1;
}

static uint16_t remove_backslashes(char* line, uint16_t line_length) {
  uint16_t head = 0;
  uint16_t tail = 0;
  uint8_t just_removed = 0;
  for (; head < line_length; ++head) {
    if (just_removed || (line[head] != '\\')) {
      line[tail] = line[head];
      ++tail;
      just_removed = 0;
    } else {
      just_removed = 1;
    }
  }
  line[tail] = '\0';
  return tail;
}

// Supported operations:
//
// Left arrow
//   027 1b 
//   091 5b [
//   068 44 D
// Right Arrow
//   027 1b 
//   091 5b [
//   067 43 C
// backspace key
//   008 08
// delete key  -- TODO
//   027 1b 
//   091 5b [
//   051 33 3
//   126 7e ~
// ctrl e
//   005 05
// ctrl c
//   003 03
// ctrl a
//   001 01
//
// everything is is ignored
static void echo(char c) {
  if (c == '\r') {
    putchar('\r');
    putchar('\n');  // also one of these
  } else if (c >= 32) {
    putchar(c);
  }
}

static void debug_echo(char c) {
  printf("%03d %02x ", c, c);
  if ((c >= 32) && (c <= 126)) {
    putchar(c);
  }
  printf("\n");
}

static inline void vt102_putchar(struct ConsoleConfig* cc, char c) {
  if (cc->mode == CONSOLE_DEBUG_VT102) {
    debug_echo(c);
  } else {
    putchar(c);
  }
}

static void vt102_backspace(struct ConsoleConfig* cc) {
  if (cc->cursor_index == 0) {
    // can't backspace
    return;
  }

  if (cc->cursor_index < cc->line_length) {
    // need to deleete a character in the middle of the buffer
    memmove(
      cc->line + cc->cursor_index - 1,
      cc->line + cc->cursor_index,
      cc->line_length - cc->cursor_index);
  }

  --cc->cursor_index;
  --cc->line_length;
  cc->tab_length = cc->line_length;
  vt102_putchar(cc, 0x08); // backspace
  vt102_putchar(cc, 0x1b); // escape
  vt102_putchar(cc, '['); // escape
  vt102_putchar(cc, '1');  // num chars
  vt102_putchar(cc, 'P');  // delete character
}

static void vt102_put_ascii_number(struct ConsoleConfig* cc, uint16_t delta) {
  if (delta == 0) {
    // nothing
    return;
  }
  char digits[8];
  uint8_t length = 0;
  for (; delta; ++length, delta /= 10) {
    digits[length] = '0' + (delta % 10);
  }
  for (; length > 0; --length) {
    vt102_putchar(cc, digits[length - 1]);
  }
}

static void vt102_beginning_of_line(struct ConsoleConfig* cc) {
  if (cc->cursor_index == 0) {
    // already at beginning
    return;
  }
  vt102_putchar(cc, 0x1b); // escape
  vt102_putchar(cc, '['); // escape
  vt102_put_ascii_number(cc, cc->cursor_index);
  vt102_putchar(cc, 'D');  // cursor left
  cc->cursor_index = 0;
}

static void vt102_end_of_line(struct ConsoleConfig* cc) {
  if (cc->cursor_index >= cc->line_length) {
    // already at eol
    return;
  }
  vt102_putchar(cc, 0x1b); // escape
  vt102_putchar(cc, '['); // escape
  vt102_put_ascii_number(cc, cc->line_length - cc->cursor_index);
  vt102_putchar(cc, 'C');  // cursor right
  cc->cursor_index = cc->line_length;
}

static void vt102_erase_current_line(struct ConsoleConfig* cc) {
  if (cc->line_length == 0) {
    // already empty
    return;
  }
  vt102_beginning_of_line(cc);
  vt102_putchar(cc, 0x1b); // escape
  vt102_putchar(cc, '['); // escape
  vt102_put_ascii_number(cc, cc->line_length);
  vt102_putchar(cc, 'P');  // delete character

  cc->line_length = 0;
  cc->line[0] = 0;
}

static void vt102_replace_current_line(struct ConsoleConfig* cc, const char* line) {
  vt102_erase_current_line(cc);
  const uint16_t new_length = strlen(line);
  memcpy(cc->line, line, new_length);
  cc->line_length = new_length;
  cc->cursor_index = new_length;
  cc->tab_length = cc->line_length;
  for (uint16_t i=0; i<new_length; ++i) {
    vt102_putchar(cc, line[i]);
  }
}


static uint8_t vt102_try_tab_complete(struct ConsoleConfig* cc, uint8_t callback_idx) {
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


static void vt102_tab_pressed(struct ConsoleConfig* cc) {
  for (uint8_t i=0; i < (cc->callback_count + 1); ++i) {
    const uint8_t callback_idx = (cc->tab_callback_index + i + 1) % (cc->callback_count + 1);
    if (vt102_try_tab_complete(cc, callback_idx)) {
      return;
    }
  } 
}


static char parse_vt102_normal(struct ConsoleConfig* cc, char c) {
  if (c >= 32) {
    // just a regular character
    vt102_putchar(cc, c);
    return c;
  }

  switch (c) {
    case '\r':
      vt102_putchar(cc, c);
      vt102_putchar(cc, '\n');
      return c;
    case '\t':
      vt102_tab_pressed(cc);
      break;
    case 0x1b:
      cc->terminal_state = VT102_ESCAPE;
      break;
    case 0x08:
      vt102_backspace(cc);
      break;
    case 0x05:
      vt102_end_of_line(cc);
      break;
    case 0x01:
      vt102_beginning_of_line(cc);
      break;
    case 0x03:
      return c; // ctrl-c
  }

  return 0;
}

#if CONSOLE_HISTORY_LINES > 0
static uint8_t maybe_push_line_to_history(struct ConsoleConfig* cc) {
  // first make sure the line is not empty
  uint8_t is_empty = 1;
  for (uint16_t i=0; i<cc->line_length; ++i) {
    if (cc->line[i] != ' ') {
      is_empty = false;
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

static void vt102_history_previous(struct ConsoleConfig* cc) {
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

static void vt102_history_next(struct ConsoleConfig* cc) {
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

static char parse_vt102_escape2(struct ConsoleConfig* cc, char c) {
  cc->terminal_state = VT102_NORMAL;
  switch (c) {
#if CONSOLE_HISTORY_LINES > 0
    case 'A':
      // up arrow
      vt102_history_previous(cc);
      return 0;
    case 'B':
      // down arrow
      vt102_history_next(cc);
      return 0;
#endif
    case 'D':
      // left arrow
      if (cc->cursor_index == 0) {
        return 0;  // already at zero
      }
      --cc->cursor_index;
      break;
    case 'C':
      // right arrow
      if (cc->cursor_index >= cc->line_length) {
        return 0;  // already at end
      }
      ++cc->cursor_index;
      break;
    default:
      // unsupported sequence
      return 0;
  }

  vt102_putchar(cc, 0x1b);
  vt102_putchar(cc, '[');
  vt102_putchar(cc, c);
  return 0;
}

static char parse_vt102(struct ConsoleConfig* cc, char c) {
  switch (cc->terminal_state) {
    case VT102_NORMAL:
      return parse_vt102_normal(cc, c);
    case VT102_ESCAPE:
      if (c == '[') {
        cc->terminal_state = VT102_ESCAPE2;
        return 0;
      }
      cc->terminal_state = VT102_NORMAL;
      return parse_vt102(cc, c);
    case VT102_ESCAPE2:
      return parse_vt102_escape2(cc, c);
  }

  return c;
}

static int split_args(struct ConsoleConfig* cc) {
  if (!convert_spaces_to_nulls(cc->line, cc->line_length)) {
    return 0;
  }
  cc->line_length = remove_backslashes(cc->line, cc->line_length);
  if (cc->line_length == 0) {
    return 1;  // nothing to do
  }

  if (cc->line[0] != 0) {
    cc->arg[0] = cc->line;
    ++cc->num_args;
  }

  for (uint16_t i=1; i<cc->line_length; ++i) {
    if ((cc->line[i] != 0) && (cc->line[i-1] == 0)) {
      if (cc->num_args >= CONSOLE_MAX_ARGS) {
        printf("Too many arguments (>%d)\n", CONSOLE_MAX_ARGS);
        return 0;
      }
      cc->arg[cc->num_args] = cc->line + i;
      if (cc->arg[cc->num_args][0] == 0xFF) {
        // special case empty quoted string
        cc->arg[cc->num_args][0] = '\0';
      }
      ++cc->num_args;
    }
  }
  
  return 1;  // ok
}

static void dump_help(const struct ConsoleConfig* cc) {
  for (uint8_t i=0; i < cc->callback_count; ++i) {
    const struct ConsoleCallback* cb = cc->callbacks + i;
    printf("%s: %s\n", cb->command, cb->description);
  }
}

static uint8_t check_arg_count(const struct ConsoleCallback* cb, uint8_t argc) {
  if ((cb->num_args >= 0) && (cb->num_args != argc)) {
    if (argc == 0) {
      printf("%s: Unexpected argument(s)\n", cb->command);
    } else if (argc == 1) {
      printf("%s: Expected 1 argument\n", cb->command);
    } else {
      printf("%s: Expected %d arguments\n", cb->command, argc);
    }
    return 0;
  }
  return 1;
}

static void parse_line(struct ConsoleConfig* cc) {
  cc->line[cc->line_length] = 0;  // null terminate the end
#if CONSOLE_HISTORY_LINES > 0
  maybe_push_line_to_history(cc);
#endif
  if (!split_args(cc)) {
    return;
  }
  if (cc->num_args == 0) {
    return;
  }
  // At this point, there should be a null termination after
  // the command.
  const char* command = cc->line;
  for (uint8_t i=0; i < cc->callback_count; ++i) {
    struct ConsoleCallback* cb = cc->callbacks + i;
    if (!strcmp(command, cb->command)) {
      if (check_arg_count(cb, cc->num_args - 1)) {
        cb->callback(cc->num_args - 1, cc->arg + 1);
      }
      return;
    }
  }

  // nothing was found,  look for "?", or "help"
  if (!strcmp(command, "?") || !strcmp(command, "help")) {
    dump_help(cc);
    return;
  }

  printf(
    "Unknown Command \"%s\".  Try ? or \"help\".\n", command);
}

static void dump_internal_state(struct ConsoleConfig* cc) {
  const char* state = "UNKNOWN";
  switch (cc->terminal_state) {
    case VT102_NORMAL:
      state = "NORMAL";
      break;
    case VT102_ESCAPE:
      state = "ESCAPE";
      break;
    case VT102_ESCAPE2:
      state = "ESCAPE2";
      break;
  }

  printf("state=%s line_length=%d cursor_index=%d\n", state, cc->line_length, cc->cursor_index);
  putchar('"');
  for (uint16_t i=0; i<cc->line_length; ++i) {
    putchar(cc->line[i]);
  }
  printf("\"\n");
  putchar(' ');
  for (uint16_t i=0; i<cc->cursor_index; ++i) {
    putchar(' ');
  }
  printf("^\n");
}

void uart_console_poll(
    struct ConsoleConfig* cc,
    const char* prompt) {
  if ((cc->prompt_displayed == 0) && (cc->mode != CONSOLE_MINIMAL)) {
    printf(prompt);
    if (cc->mode == CONSOLE_VT102) {
      // put the terminal in insert mode
      // This is done every line in case the terminal was disconnected or reset.
      putchar(0x1b);
      putchar('[');
      putchar('4');
      putchar('h');
    }
    cc->prompt_displayed = 1;
  }

  const int cint = getchar_timeout_us(0);
  if ((cint < 0) || (cint > 127)) {
    // didn't get anything
    return;
  }

  char c = (char)cint;

  switch (cc->mode) {
    case CONSOLE_MINIMAL:
      // nothing to do
      break;
    case CONSOLE_ECHO:
      echo(c);
      break;
    case CONSOLE_DEBUG_ECHO:
      debug_echo(c);
      break;
    case CONSOLE_VT102:
    case CONSOLE_DEBUG_VT102:
      c = parse_vt102(cc, c);
      break;
  }

  if (c == '\r') {
    parse_line(cc);
    reset_line(cc);
  } else if (c == 0x03) {
    // ctrl-c
    printf("\nCancelled\n");
    reset_line(cc);  
  } else if (c < 32) {
    // ignore this code
  } else if (cc->line_length >= CONSOLE_MAX_LINE_CHARS) {
    printf("\nLine too long (>%d characters)\n", CONSOLE_MAX_LINE_CHARS);
    reset_line(cc);
  } else {
    if (cc->line_length > cc->cursor_index) {
      // need to insert a charactrer into line 
      // make a spot for it
      memmove(
        cc->line + cc->cursor_index + 1,
        cc->line + cc->cursor_index,
        cc->line_length - cc->cursor_index);
    } 

    // fill it in
    cc->line[cc->cursor_index] = c;

    // update indexes
    ++cc->line_length;
    ++cc->cursor_index;
    cc->tab_length = cc->line_length;
  }

  if (cc->mode == CONSOLE_DEBUG_VT102) {
    dump_internal_state(cc);
  }
}
