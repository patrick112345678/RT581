sdk_generate_component_tag()

if(${CHIP_RF} STREQUAL "rt569")
    sdk_add_subdirectory_ifdef(CONFIG_RF ${CMAKE_CURRENT_LIST_DIR}/rt569_rf)
    sdk_add_subdirectory_ifdef(CONFIG_MP ${CMAKE_CURRENT_LIST_DIR}/rt569mp)
endif()


sdk_add_subdirectory_ifdef(CONFIG_RUCI ${CMAKE_CURRENT_LIST_DIR}/ruci)
sdk_add_subdirectory_ifdef(CONFIG_LMAC15P4 ${CMAKE_CURRENT_LIST_DIR}/lmac15p4)
sdk_add_subdirectory_ifdef(CONFIG_BLUETOOTH ${CMAKE_CURRENT_LIST_DIR}/bluetooth)
sdk_add_subdirectory_ifdef(CONFIG_THREAD ${CMAKE_CURRENT_LIST_DIR}/thread)