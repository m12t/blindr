file(GLOB FILES *.c *.h)

add_library(stepper ${FILES})

target_link_libraries(stepper pico_stdlib)

target_include_directories(stepper PUBLIC .)
