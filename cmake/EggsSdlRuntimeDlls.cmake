# Eggs.SDL
#
# Copyright (c) 2026 Agustin Berge
#
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt

include_guard()

# eggs_sdl_copy_runtime_dlls(<target>)
#
# On Windows, copies the SDL3 DLL next to the executable <target>, so that it
# runs without SDL3's bin directory on PATH.
function(eggs_sdl_copy_runtime_dlls target)
    if(WIN32 AND TARGET SDL3::SDL3-shared)
        add_custom_command(
            TARGET ${target}
            POST_BUILD
            COMMAND
                ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:${target}> $<TARGET_FILE_DIR:${target}>
            COMMAND_EXPAND_LISTS
        )
    endif()
endfunction()
