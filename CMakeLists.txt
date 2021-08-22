cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(submodules/pico-sdk/external/pico_sdk_import.cmake)
include(submodules/freertos-kernel/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

project(pico_freertos C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
include_directories(freertospp/include)

# Initialize the SDK
pico_sdk_init()

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
)

add_subdirectory(freertospp)
add_subdirectory(blinky_vanilla)
add_subdirectory(blinky_cpp)
add_subdirectory(queue_basic_cpp)
add_subdirectory(analog)
