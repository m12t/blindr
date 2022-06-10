add_executable(gnss)

pico_generate_pio_header(gnss ${CMAKE_CURRENT_LIST_DIR}/gnss.pio)

target_sources(gnss PRIVATE gnss.c)

target_link_libraries(gnss PRIVATE
        pico_stdlib
        pico_multicore
        hardware_pio
        hardware_dma
        )

pico_add_extra_outputs(gnss)

# add url via pico_set_program_url
example_auto_set_url(gnss)
