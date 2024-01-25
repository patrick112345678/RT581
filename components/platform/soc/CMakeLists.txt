if((${CHIP} STREQUAL "rt582"))
    sdk_add_subdirectory_ifdef(CONFIG_DRIVER ${CMAKE_CURRENT_LIST_DIR}/rt582/rt582_driver)
    sdk_add_subdirectory_ifdef(CONFIG_FREERTOS ${CMAKE_CURRENT_LIST_DIR}/rt582/rt582_freertos)
    sdk_add_subdirectory_ifdef(CONFIG_CRYPTO ${CMAKE_CURRENT_LIST_DIR}/rt582/rt582_crypto)
    
    if((${BOARD} STREQUAL "dongle"))
        sdk_add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/rt582/rt582_dongle)
    elseif((${BOARD} STREQUAL "evb"))
        sdk_add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/rt582/rt582_evb)
    endif()
elseif((${CHIP} STREQUAL "rt581"))

endif()

