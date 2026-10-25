// Eggs.SDL
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#include <eggs/sdl.hpp>

#include <cstdlib>

// Links against the library without needing a display, run by the consumer
// tests.
int main()
{
    namespace sdl = eggs::sdl;

    if (sdl::was_init() != sdl::init_flags::none) return EXIT_FAILURE;

    sdl::event const event;
    if (!event.holds_alternative<sdl::unknown_event>()) return EXIT_FAILURE;

    if (sdl::window{}) return EXIT_FAILURE;
    return EXIT_SUCCESS;
}
