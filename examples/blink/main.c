#include "pico/stdlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "uart_console/console.h"

//#define LED_PIN 11  // Adafruit Itsy RP2040
#define LED_PIN PICO_DEFAULT_LED_PIN

uint8_t stop_sleeping;
uint32_t led_on_ms;
uint32_t led_off_ms;

// Converts a string argument to an integer and sets target.
static void get_ms(char* arg, uint32_t* target) {
  int ms = atoi(arg);
  if (ms <= 0) {
    printf("Expected a positive integer\n");
  } else {
    *target = ms;
    stop_sleeping = 1;
  }
}

// changes on duration
static void on_ms(uint8_t argc, char* argv[]) {
  get_ms(argv[0], &led_on_ms);
}

// changes off duration
static void off_ms(uint8_t argc, char* argv[]) {
  get_ms(argv[0], &led_off_ms);
}

// dumps current state
static void state(uint8_t argc, char* argv[]) {
  printf("on=%dms of=%dms\n", led_on_ms, led_off_ms);
}

// Configuration to register with uart_console_init()
struct ConsoleCallback callbacks[] = {
    {"on_ms", "On time in ms", 1, on_ms},
    {"off_ms", "Off time in ms", 1, off_ms},
    {"state", "Dump current state", 0, state},
};

#define SLEEP_STEP_MS 50
// A sleep function that also calls uart_console_poll() to keep the
// interface responsive.
static void sleep_with_poll(struct ConsoleConfig* cc, uint32_t ms) {
  // This will sleep a bit longer than ms due to the function assuming that
  // everything outside of sleeping takes 0ms - but it's just
  // an example so it's tolerated for simplicity's sake.
  stop_sleeping = 0;
  for (uint32_t elapsed_ms = SLEEP_STEP_MS;
       !stop_sleeping && (elapsed_ms <= ms);
       elapsed_ms += SLEEP_STEP_MS) {
    uart_console_poll(cc, "> ");
    sleep_ms(SLEEP_STEP_MS);
  }
  uart_console_poll(cc, "> ");
  sleep_ms(ms % SLEEP_STEP_MS);
}

// program entry point
int main() {
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  led_on_ms = 500;
  led_off_ms = 500;

  struct ConsoleConfig cc;
  uart_console_init(
      &cc,
      callbacks,
      sizeof(callbacks) / sizeof(callbacks[0]),
      CONSOLE_VT102);

  while (1) {
    gpio_put(LED_PIN, 1);
    sleep_with_poll(&cc, led_on_ms); 
    gpio_put(LED_PIN, 0);
    sleep_with_poll(&cc, led_off_ms); 
  }
  return 0;
}
