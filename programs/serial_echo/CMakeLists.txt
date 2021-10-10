add_executable(serial_echo src/main.cpp)
# enable usb output, disable uart output
pico_enable_stdio_usb(serial_echo 0)
pico_enable_stdio_uart(serial_echo 1)
target_include_directories(serial_echo PUBLIC include submodules/freertos-kernel/include submodules/freertos-kernel/portable/ThirdParty/GCC/RP2040/include)
target_link_libraries(serial_echo freertoscpp_rp2040_serial_io_stdout)
target_link_libraries(serial_echo pico_stdlib)
target_link_libraries(serial_echo FreeRTOS-Kernel FreeRTOS-Kernel-Core FreeRTOS-Kernel-Heap4)
pico_add_extra_outputs(serial_echo)
