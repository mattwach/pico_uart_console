add_subdirectory(blink)
add_subdirectory(minimal)
# This may not exist if submodules were not initialized
if (EXISTS ${CMAKE_CURRENT_LIST_DIR}/sharpmem_display/pico_sharpmem_display/CMakeLists.txt)
  add_subdirectory(sharpmem_display)
endif()

