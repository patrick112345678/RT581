# add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/utility/cli)
# add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/utility/log)

sdk_generate_library(rt582_hosal)

sdk_add_include_directories(
    Inc
)

sdk_library_add_sources(
    Src/hosal_uart.c
    Src/hosal_rf.c
)