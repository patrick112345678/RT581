
if((${CHIP} STREQUAL "rt582"))
    sdk_add_subdirectory_ifdef(CONFIG_HOSAL ${CMAKE_CURRENT_LIST_DIR}/rt582_hosal)
endif()

