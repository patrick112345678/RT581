if(NOT DEFINED ENV{RAFAEL_SDK_BASE})
    message( "please set RAFAEL_SDK_BASE in your system environment")
endif()

if(MINGW OR CYGWIN OR WIN32)
SET(CMAKE_SYSTEM_NAME Generic)
elseif(UNIX)
SET(CMAKE_SYSTEM_NAME Linux)
elseif(APPLE)
SET(CMAKE_SYSTEM_NAME Darwin)
endif()
SET(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR ARM)

set(RAFAEL_SDK_BASE $ENV{RAFAEL_SDK_BASE})

set(build_dir ${CMAKE_CURRENT_BINARY_DIR}/build_out)
set(PROJECT_SOURCE_DIR ${RAFAEL_SDK_BASE})
set(PROJECT_BINARY_DIR ${build_dir})
set(EXECUTABLE_OUTPUT_PATH ${build_dir})
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

add_library(sdk_intf_lib INTERFACE)

message( "=========================================")
message( "USE Rafael base path: ${RAFAEL_SDK_BASE}" )
message( "=========================================")
include(${RAFAEL_SDK_BASE}/cmake/toolchain.cmake)
include(${RAFAEL_SDK_BASE}/cmake/extension.cmake)
include(${RAFAEL_SDK_BASE}/cmake/compiler_flags.cmake)

add_subdirectory(${RAFAEL_SDK_BASE} ${build_dir})