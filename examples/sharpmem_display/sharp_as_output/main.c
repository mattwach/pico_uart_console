#include "pico/stdlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sharpdisp/sharpconsole.h"
#include "fonts/liberation_mono_14.h"
#include "uart_console/console.h"

#define WIDTH 400
#define HEIGHT 240

struct ConsoleConfig cc;
uint8_t disp_buffer[BITMAP_SIZE(WIDTH, HEIGHT)];
struct SharpDisp sd;
struct Console console;  // for sharpdisp

static void hello(uint8_t argc, char* argv[]) {
  sharpconsole_printf(&console, "Hello World!\n");
  sharpconsole_flush(&console);
}

// Configuration to register with uart_console_init()
struct ConsoleCallback callbacks[] = {
    {"hello", "Welcome message", 0, hello},
};

static void sharp_init(void) {
  sleep_ms(100);  // allow voltage to stabilize
  sharpdisp_init(
      &sd,
      disp_buffer,
      WIDTH,
      HEIGHT,
      0x00,
//      1,  // GP1 for CS (Adafruit RP2040)
      17,  // GP17 for CS (PI Pico)
      18,  // GP18 for SCK
      19,  // GP19 for MOSI
      spi0,
      10000000
  );
  sharpconsole_init(&console, &sd, liberation_mono_14, 32);
  sharpconsole_printf(&console, "Console Initialized\n");
  sharpconsole_flush(&console);
}

static int sharp_putchar(int c) {
  sharpconsole_char(&console, c);
}

// program entry point
int main() {
  sleep_ms(500); // let sharp display power up
  sharp_init();
  uart_console_init_lowlevel(
      &cc,
      callbacks,
      sizeof(callbacks) / sizeof(callbacks[0]),
      CONSOLE_VT102,
      sharp_putchar);


  while (1) {
    uart_console_poll(&cc, "$ ");
    sleep_ms(20);
  }
  return 0;
}
