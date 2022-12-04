#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>
#include "console.h"

#define LED_PIN PICO_DEFAULT_LED_PIN;

uint8_t started;
uint8_t stop_sleeping;
uint32_t led_on_ms;
uint32_t led_off_ms;

static uint8_t check_no_args(uint8_t argc) {
  if (argc > 0) {
    printf("Unexpected args\n", command);
    return 0;
  }
  return 1;
}

static int32_t get_ms(uint8_t argc, char* argv[]) {
  if (argc != 1) {
    printf("Expected 1 argument (milliseconds)\n");
    return -1;
  }
  if (!strcmp(argv[0], "0")) {
    return 0;
  }
  int val = atoi(argv[0]);
  if (val <= 0) {
    printf("Expected a positive integer\n");
    return -1;
  }
  return val;
}

static void start(uint8_t argc, char* argv[]) {
  if (check_no_args(argc)) {
    started = 1;
    stop_sleeping = 1;
  }
}

static void stop(uint8_t argc, char* argv[]) {
  if (check_no_args(argc)) {
    started = 0;
    stop_sleeping = 1;
  }
}

static void on_ms(uint8_t argc, char* argv[]) {
  int32_t ms = get_ms(argc, argv[0]);
  if (ms >= 0) {
    led_on_ms = ms;
    stop_sleeping = 1;
  }
}

static void off_ms(uint8_t argc, char* argv[]) {
  int32_t ms = get_ms(argc, argv[0]);
  if (ms >= 0) {
    led_off_ms = ms;
    stop_sleeping = 1;
  }
}

static void state(uint8_t argc, char* argv[]) {
  if (check_no_args(argc)) {
    printf("%s\n", started ? "started" : "stopped");
    printf("on=%dms of=%dms\n", led_on_ms, led_off_ms);
  }
}

struct ConsoleCallback callbacks[] = {
    {"start", "Start Blinking", start},
    {"start", "Stop Blinking", stop},
    {"on_ms", "On time in ms", on_ms},
    {"off_ms", "Off time in ms", off_ms},
    {"state", "Dump current state", state},
};

int main() {
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  led_on_ms = 500;
  led_off_ms = 500;

  struct ConsoleConfig cc;
  console_init(
      &cc,
      callbacks,
      sizeof(callbacks) / sizeof(callbacks[0]),
      CONSOLE_VT102);

  while (1) {
    sleep_with_poll(&cc, 1, led_on_ms); 
    // TODO: I am here
  }
  return 0;
}
