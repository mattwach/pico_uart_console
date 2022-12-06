add_executable(uart_console_minimal
        main.c
        )

# pull in common dependencies
target_link_libraries(
    uart_console_minimal
    UART_CONSOLE
    pico_stdlib)

# enable usb output, disable uart output
pico_enable_stdio_usb(uart_console_minimal 1)
pico_enable_stdio_uart(uart_console_minimal 0)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(uart_console_minimal)
