file(GLOB FILES *.c *.h)

add_library(gnss ${FILES})

target_link_libraries(gnss pico_stdlib hardware_uart hardware_rtc utils)

target_include_directories(gnss PUBLIC .)
