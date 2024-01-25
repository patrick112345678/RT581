message("Building components:")

sdk_add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/libc)
sdk_add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/utility)
sdk_add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/network)
sdk_add_subdirectory_ifdef(CONFIG_CPC ${CMAKE_CURRENT_LIST_DIR}/cpc)
sdk_add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/platform)