cmake_minimum_required(VERSION 3.15)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
ENABLE_LANGUAGE(C CXX ASM)
include(proj_default.conf)

sdk_value_ifdef(CHIP)
sdk_value_ifdef(BOARD)

set(RAFAEL_SDK_VER "")
get_git_hash(RAFAEL_SDK_VER)
message(STATUS "Current SDK hash is ${RAFAEL_SDK_VER}")

sdk_add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/components)
