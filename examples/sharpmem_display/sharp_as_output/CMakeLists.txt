add_executable(uart_console_sharp_as_output
        main.c
        )

# pull in common dependencies
target_link_libraries(
    uart_console_sharp_as_output
    pico_stdlib
    UART_CONSOLE
    SHARPCONSOLE
    SHARPDISP
    LIBERATION_MONO_14
)

# enable usb output, disable uart output
pico_enable_stdio_usb(uart_console_sharp_as_output 1)
pico_enable_stdio_uart(uart_console_sharp_as_output 0)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(uart_console_sharp_as_output)
