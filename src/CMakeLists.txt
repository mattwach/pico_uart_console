cmake_minimum_required(VERSION 3.12)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

add_library(UART_CONSOLE INTERFACE)
target_include_directories(UART_CONSOLE  INTERFACE ${CMAKE_CURRENT_LIST_DIR}/../include)
target_sources(UART_CONSOLE  INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/uart_console.c
    ${CMAKE_CURRENT_LIST_DIR}/vt102_tab_complete.c
    ${CMAKE_CURRENT_LIST_DIR}/vt102_util.c
    ${CMAKE_CURRENT_LIST_DIR}/util.c
)
