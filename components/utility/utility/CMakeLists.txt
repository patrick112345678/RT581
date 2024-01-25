

sdk_generate_library(utility)

sdk_add_include_directories(
    Inc
)

sdk_library_add_sources(
    util_list.c
    util_log.c
)