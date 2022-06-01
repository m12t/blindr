file(GLOB FILES *.c *.h)

add_library(solar ${FILES})

target_link_libraries(solar)

target_include_directories(solar PUBLIC .)
