file(GLOB FILES *.c *.h)

add_library(setup ${FILES})

target_link_libraries(setup pico_stdlib hardware_uart)

target_include_directories(setup PUBLIC .)
