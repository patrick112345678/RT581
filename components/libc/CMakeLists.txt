sdk_generate_component_tag()

sdk_ifndef(CONFIG_NEWLIBC false)

sdk_add_subdirectory_ifdef(CONFIG_NEWLIBC ${CMAKE_CURRENT_LIST_DIR}/newlibc)