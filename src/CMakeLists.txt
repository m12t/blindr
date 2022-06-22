cmake_minimum_required(VERSION 3.12)

# Pull in SDK (`include()` must be called before `project()`)
include(pico_sdk_import.cmake)

project(blindr VERSION 0.1.0)
project(blindr C)
set(CMAKE_C_STANDARD 11)

pico_sdk_init()

add_subdirectory(./utils)
add_subdirectory(./gnss)
add_subdirectory(./stepper)
add_subdirectory(./toggle)
add_subdirectory(./solar)


add_executable(blindr blindr.c)
target_link_libraries(blindr utils gnss stepper toggle solar)  # rtc_alarm?

pico_enable_stdio_usb(blindr 1)  # 0 before deployment - also remove all printf() statements
pico_enable_stdio_uart(blindr 0) # 0 before deployment
# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(blindr)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
