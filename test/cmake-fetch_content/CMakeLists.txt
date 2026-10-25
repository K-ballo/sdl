# Eggs.SDL
#
# Copyright (c) 2026 Agustin Berge
#
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt

cmake_minimum_required(VERSION 4.0...4.4.0)
project(Eggs.SDL-FetchContent LANGUAGES CXX)

set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
    "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>"
)

# EGGS_SDL_SOURCE_DIR is injected by CI to point at the local tree.
if(NOT DEFINED EGGS_SDL_SOURCE_DIR)
    message(
        FATAL_ERROR
        "EGGS_SDL_SOURCE_DIR must be set. "
        "Pass -DEGGS_SDL_SOURCE_DIR=<path> to cmake."
    )
endif()
file(TO_CMAKE_PATH "${EGGS_SDL_SOURCE_DIR}" EGGS_SDL_SOURCE_DIR)

include(FetchContent)
FetchContent_Declare(Eggs.SDL SOURCE_DIR "${EGGS_SDL_SOURCE_DIR}")
FetchContent_MakeAvailable(Eggs.SDL)

add_executable(consumer_verify main.cpp)
target_link_libraries(consumer_verify PRIVATE Eggs::SDL)
if(WIN32 AND TARGET SDL3::SDL3-shared)
    add_custom_command(
        TARGET consumer_verify
        POST_BUILD
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:consumer_verify>
            $<TARGET_FILE_DIR:consumer_verify>
        COMMAND_EXPAND_LISTS
    )
endif()

foreach(
    _prop
    IN
    ITEMS
        INTERFACE_INCLUDE_DIRECTORIES
        INTERFACE_COMPILE_FEATURES
        INTERFACE_LINK_LIBRARIES
)
    get_target_property(_val Eggs::SDL ${_prop})
    message(STATUS "Eggs::SDL (FetchContent) ${_prop}: ${_val}")
endforeach()

enable_testing()
add_test(NAME verify COMMAND consumer_verify)
