file(GLOB FILES *.c *.h)

add_library(utils ${FILES})

target_link_libraries(utils pico_stdlib hardware_uart hardware_rtc)

target_include_directories(utils PUBLIC .)
