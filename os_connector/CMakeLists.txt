cmake_minimum_required(VERSION 3.20)

project(ImNANDConnector C)

include (libs/ufedmd.cmake)

set(UFEDMD_EXTERNAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/ufedmd)
set(UFEDMD_SRC_DIR ${UFEDMD_EXTERNAL_DIR}/src)

message(STATUS "C compiler: ${CMAKE_C_COMPILER}")
message(STATUS "System: ${CMAKE_SYSTEM_NAME}")

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/server/main.c 
            ${CMAKE_CURRENT_SOURCE_DIR}/server/params.c
            ${CMAKE_CURRENT_SOURCE_DIR}/server/hotplug_queue.c
            ${CMAKE_CURRENT_SOURCE_DIR}/server/demo/main.c)

# FIXME: Add a flag to select other platforms (like Windows, or possibly
# other Linux libc's)
set(PLATFORM_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/server/linux/main.c)

add_executable(ImNANDConnector ${SOURCES} ${PLATFORM_SOURCES})

# We don't include jansson or ufedm-dev - these are dependencies of ufedmd
# and therefore are built as well.
add_dependencies(ImNANDConnector ufedmd)

target_compile_options(ImNANDConnector PRIVATE -Wall -Wextra -Werror)

target_include_directories(ImNANDConnector PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/jansson/src
        ${UFEDMD_SRC_DIR})

target_link_options(ImNANDConnector PRIVATE -static)

# Optional: debug build support
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(ImNANDConnector PRIVATE -O0 -g)
endif()
