add_executable(uart_console_show_internal_sate
        main.c
        )

# pull in common dependencies
target_link_libraries(
    uart_console_show_internal_sate
    pico_stdlib
    UART_CONSOLE
    SHARPCONSOLE
    SHARPDISP
    LIBERATION_MONO_14
)

# enable usb output, disable uart output
pico_enable_stdio_usb(uart_console_show_internal_sate 1)
pico_enable_stdio_uart(uart_console_show_internal_sate 0)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(uart_console_show_internal_sate)
