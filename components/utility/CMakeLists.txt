sdk_generate_component_tag()

sdk_add_subdirectory_ifdef(CONFIG_UTILITY ${CMAKE_CURRENT_LIST_DIR}/utility)
sdk_add_subdirectory_ifdef(CONFIG_LOG ${CMAKE_CURRENT_LIST_DIR}/log)
sdk_add_subdirectory_ifdef(CONFIG_UART_STDIO ${CMAKE_CURRENT_LIST_DIR}/uart_stdio)
sdk_add_subdirectory_ifdef(CONFIG_CLI ${CMAKE_CURRENT_LIST_DIR}/cli)
sdk_add_subdirectory_ifdef(CONFIG_ENHANCEFLASHDATASET ${CMAKE_CURRENT_LIST_DIR}/EnhancedFlashDataset)
