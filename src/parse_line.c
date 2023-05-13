
// Contains logic for parsing a completed string into arguments
// as well as invoking the callback function
#include "parse_line.h"
#include "util.h"
#include "command_history.h"
#include <stdio.h>
#include <string.h>

// Dumps command help to the screen
static void dump_help(const struct ConsoleConfig* cc) {
  for (uint8_t i=0; i < cc->callback_count; ++i) {
    const struct ConsoleCallback* cb = cc->callbacks + i;
    console_printf(cc, "%s: %s\n", cb->command, cb->description);
  }
}

// Makes sure the number of provided arguments is what the command
// is expecting.
static uint8_t check_arg_count(
  const struct ConsoleConfig* cc,
  const struct ConsoleCallback* cb,
  uint8_t argc) {
  if ((cb->num_args >= 0) && (cb->num_args != argc)) {
    if (cb->num_args == 0) {
      console_printf(cc, "%s: Unexpected argument(s)\n", cb->command);
    } else if (cb->num_args == 1) {
      console_printf(cc, "%s: Expected 1 argument\n", cb->command);
    } else {
      console_printf(cc, "%s: Expected %d arguments\n", cb->command, cb->num_args);
    }
    return 0;
  }
  return 1;
}

// Converts spaces in cc->line to null characters.
// There is some additional complication that stems from supporting
// quotes "" and backslash characters.
static int convert_spaces_to_nulls(
  const struct ConsoleConfig* cc,
  char* line,
  uint16_t line_length) {
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
    console_printf(cc, "Unclosed quote\n");
    return 0;
  }
  if (backslash) {
    console_printf(cc, "Line ended with backslash\n");
    return 0;
  }
  return 1;
}


// At this point, backslashes have protected special characters,
// such as spaces and quotes, from being interpreted in their
// usual way.  This post steps removes the backslashes from
// the string.
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

// Sets up cc->args by replacing whitespace
// with nulls and setting appropriate args[] pointers
// to point within cc->line.
//
// Returns the number of arguments found, including the
// command itself (this zero represents an error condition).
static int split_args(struct ConsoleConfig* cc) {
  int num_args = 0;
  if (!convert_spaces_to_nulls(cc, cc->line, cc->line_length)) {
    return 0;
  }
  cc->line_length = remove_backslashes(cc->line, cc->line_length);
  if (cc->line_length == 0) {
    return 0;  // nothing to do
  }

  if (cc->line[0] != 0) {
    cc->arg[0] = cc->line;
    ++num_args;
  }

  for (uint16_t i=1; i<cc->line_length; ++i) {
    if ((cc->line[i] != 0) && (cc->line[i-1] == 0)) {
      if (num_args >= CONSOLE_MAX_ARGS) {
        console_printf(cc, "Too many arguments (>%d)\n", CONSOLE_MAX_ARGS);
        return 0;
      }
      cc->arg[num_args] = cc->line + i;
      if (cc->arg[num_args][0] == 0xFF) {
        // special case empty quoted string
        cc->arg[num_args][0] = '\0';
      }
      ++num_args;
    }
  }
  
  return num_args;  // ok
}

void uart_console_parse_line(struct ConsoleConfig* cc) {
  cc->line[cc->line_length] = 0;  // null terminate the end
#if CONSOLE_HISTORY_LINES > 0
  maybe_push_line_to_history(cc);
#endif
  const int num_args = split_args(cc);
  if (num_args == 0) {
    return;
  }
  // At this point, there should be a null termination after
  // the command.
  const char* command = cc->line;
  for (uint8_t i=0; i < cc->callback_count; ++i) {
    struct ConsoleCallback* cb = cc->callbacks + i;
    if (!strcmp(command, cb->command)) {
      if (check_arg_count(cc, cb, num_args - 1)) {
        cb->callback(num_args - 1, cc->arg + 1);
      }
      return;
    }
  }

  // nothing was found,  look for "?", or "help"
  if (!strcmp(command, "?") || !strcmp(command, "help")) {
    dump_help(cc);
    return;
  }

  console_printf(
    cc,
    "Unknown Command \"%s\".  Try ? or \"help\".\n", command);
}
