add_library(freertoscpp_rp2040_serial_io_stdout INTERFACE)
target_sources(freertoscpp_rp2040_serial_io_stdout INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/src/serial_io.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/rp2040_serial_io_stdout.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/rp2040_hw_timer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/modbus_daemon.cpp
)
target_include_directories(freertoscpp_rp2040_serial_io_stdout INTERFACE include)
target_link_libraries(freertoscpp_rp2040_serial_io_stdout INTERFACE pico_stdlib)


add_library(freertoscpp_rp2040_adcirq INTERFACE)
target_sources(freertoscpp_rp2040_adcirq INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/adc.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/crc16.c
)
target_include_directories(freertoscpp_rp2040_adcirq INTERFACE include)
target_link_libraries(freertoscpp_rp2040_adcirq INTERFACE pico_stdlib hardware_adc)
