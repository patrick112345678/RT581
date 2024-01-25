sdk_generate_component_tag()
sdk_generate_library(cpc)

sdk_add_include_directories(
    inc
)

sdk_library_add_sources(
    src/cpc_crc.c
    src/cpc_dispatcher.c
    src/cpc_drv_secondary_uart_usart.c
    src/cpc_hdlc.c
    src/cpc_system_secondary.c
    src/cpc.c
    src/cpc_timer.c
    src/slist.c
    src/mem_pool.c
    src/cpc_memory.c
)

