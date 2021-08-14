add_executable(pico_freertos_cpp src/main.cpp)
# enable usb output, disable uart output
pico_enable_stdio_usb(pico_freertos_cpp 1)
pico_enable_stdio_uart(pico_freertos_cpp 0)
target_include_directories(pico_freertos_cpp PUBLIC include submodules/freertos-kernel/include submodules/freertos-kernel/portable/ThirdParty/GCC/RP2040/include)
target_link_libraries(pico_freertos_cpp pico_stdlib)
target_link_libraries(pico_freertos_cpp FreeRTOS-Kernel FreeRTOS-Kernel-Core FreeRTOS-Kernel-Heap4)
pico_add_extra_outputs(pico_freertos_cpp)
