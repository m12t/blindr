file(GLOB FILES *.c *.h *.pio)

add_library(gnss ${FILES})
pico_generate_pio_header(gnss ${CMAKE_CURRENT_LIST_DIR}/gnss.pio)
target_sources(gnss PRIVATE gnss.c)

target_link_libraries(gnss pico_stdlib hardware_uart hardware_dma hardware_pio utils)

target_include_directories(gnss PUBLIC .)
