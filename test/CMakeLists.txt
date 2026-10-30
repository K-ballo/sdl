# Eggs.SDL
#
# Copyright (c) 2026 Agustin Berge
#
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt

# -- Verify interface header sets ----------------------------------------------

add_test(
    NAME eggs.sdl.verify_headers
    COMMAND
        ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target
        _eggs_sdl_verify_interface_header_sets --config $<CONFIG>
)
set_tests_properties(
    eggs.sdl.verify_headers
    PROPERTIES LABELS "header" RESOURCE_LOCK cmake_build
)

# -- Verify definitions in headers -------------------------------------------

include(AddMultipleDefinitionsTest)
include(EggsSdlRuntimeDlls)

add_multiple_definitions_test(eggs.sdl.verify_definitions _eggs_sdl)
set_target_properties(eggs.sdl.verify_definitions PROPERTIES FOLDER "header")
set_tests_properties(
    eggs.sdl.verify_definitions
    PROPERTIES LABELS "header" RESOURCE_LOCK cmake_build
)

# Helper - creates a test executable and registers it with CTest.
function(add_lib_test name)
    add_executable(eggs.sdl.test.${name} ${name}.cpp)
    target_sources(
        eggs.sdl.test.${name}
        PRIVATE
            FILE_SET private_headers
                TYPE HEADERS
                BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
                FILES support.hpp
    )
    target_link_libraries(eggs.sdl.test.${name} PRIVATE Eggs::SDL ${ARGN})
    eggs_sdl_copy_runtime_dlls(eggs.sdl.test.${name})

    add_test(NAME eggs.sdl.test.${name} COMMAND eggs.sdl.test.${name})
    set_tests_properties(
        eggs.sdl.test.${name}
        PROPERTIES
            LABELS "unit"
            ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy"
    )
endfunction()

# -- Unit tests ----------------------------------------------------------------

find_package(Threads REQUIRED)

add_lib_test(types)
add_lib_test(init)
add_lib_test(window)
add_lib_test(event)
add_lib_test(surface)
add_lib_test(time)
add_lib_test(renderer)
add_lib_test(window_demo Threads::Threads)
add_lib_test(surface_demo Threads::Threads)
add_lib_test(render_demo Threads::Threads)
target_include_directories(
    eggs.sdl.test.render_demo
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../example
)
target_include_directories(
    eggs.sdl.test.window_demo
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../example
)
target_include_directories(
    eggs.sdl.test.surface_demo
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../example
)
