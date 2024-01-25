

sdk_ifndef(CONFIG_HCI_INTERFACE false)

sdk_add_subdirectory_ifdef(CONFIG_HCI_INTERFACE ${CMAKE_CURRENT_LIST_DIR}/HCI_Interface)