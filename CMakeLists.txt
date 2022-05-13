cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

project(blindr VERSION 0.1.0)
set(CMAKE_C_STANDARD 11)

include(CTest)
enable_testing()

add_subdirectory(src)

pico_sdk_init()

target_link_libraries(pico_stdlib)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
