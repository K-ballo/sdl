# Eggs.SDL
#
# Copyright (c) 2026 Agustin Berge
#
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt

include(EggsSdlRuntimeDlls)

# The demos need a window and wait for the user, so they are built but not
# registered as tests; test/window_demo.cpp drives window_demo.hpp.
function(add_example name)
    add_executable(eggs.sdl.example.${name} ${name}.cpp)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${name}.hpp)
        target_sources(
            eggs.sdl.example.${name}
            PRIVATE
                FILE_SET private_headers
                    TYPE HEADERS
                    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
                    FILES ${name}.hpp
        )
    endif()
    target_link_libraries(eggs.sdl.example.${name} PRIVATE ${ARGN})
    eggs_sdl_copy_runtime_dlls(eggs.sdl.example.${name})
endfunction()

add_example(window_demo Eggs::SDL)

# Consumer smoke test, runs without a display.
add_executable(eggs.sdl.example.main main.cpp)
target_link_libraries(eggs.sdl.example.main PRIVATE Eggs::SDL)
eggs_sdl_copy_runtime_dlls(eggs.sdl.example.main)
if(BUILD_TESTING)
    add_test(NAME eggs.sdl.example.main COMMAND eggs.sdl.example.main)
    set_tests_properties(eggs.sdl.example.main PROPERTIES LABELS "example")
endif()
