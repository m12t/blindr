file(GLOB FILES *.c *.h)

add_library(toggle ${FILES})

target_link_libraries(toggle pico_stdlib hardware_gpio)

target_include_directories(toggle PUBLIC .)
