add_executable(pico_freertos_rtu_slave src/main.cpp)
# enable usb output, disable uart output
pico_enable_stdio_usb(pico_freertos_rtu_slave 1)
pico_enable_stdio_uart(pico_freertos_rtu_slave 0)
target_include_directories(pico_freertos_rtu_slave PUBLIC include submodules/freertos-kernel/include submodules/freertos-kernel/portable/ThirdParty/GCC/RP2040/include)
target_link_libraries(
    pico_freertos_rtu_slave
    freertoscpp_rp2040_adcirq
    freertoscpp_rp2040_serial_io_stdout
)
target_link_libraries(pico_freertos_rtu_slave FreeRTOS-Kernel FreeRTOS-Kernel-Core FreeRTOS-Kernel-Heap4)
pico_add_extra_outputs(pico_freertos_rtu_slave)
