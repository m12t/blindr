file(GLOB FILES *.c *.h)

add_library(solar ${FILES})

target_link_libraries(solar pico_stdlib hardware_rtc)

target_include_directories(solar PUBLIC .)
