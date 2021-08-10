cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(submodules/pico-sdk/external/pico_sdk_import.cmake)
include(submodules/freertos-kernel/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

project(pico_freertos C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
pico_sdk_init()

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
)

add_executable(pico_freertos src/main.cpp)
# enable usb output, disable uart output
pico_enable_stdio_usb(pico_freertos 1)
pico_enable_stdio_uart(pico_freertos 0)
target_include_directories(pico_freertos PUBLIC include submodules/freertos-kernel/include submodules/freertos-kernel/portable/ThirdParty/GCC/RP2040/include)
target_link_libraries(pico_freertos pico_stdlib)
target_link_libraries(pico_freertos FreeRTOS-Kernel FreeRTOS-Kernel-Core FreeRTOS-Kernel-Heap4)
pico_add_extra_outputs(pico_freertos)
