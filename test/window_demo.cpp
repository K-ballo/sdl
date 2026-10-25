// Eggs.SDL
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#include "window_demo.hpp"

#include <eggs/sdl.hpp>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <thread>

#include "support.hpp"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_scancode.h>

namespace sdl = eggs::sdl;

namespace {

auto make_key_down(SDL_Scancode scancode) -> sdl::event
{
    sdl::event result;
    result.sdl().type = SDL_EVENT_KEY_DOWN;
    result.sdl().key.scancode = scancode;
    result.sdl().key.down = true;
    return result;
}

auto make(std::uint32_t type) -> sdl::event
{
    sdl::event result;
    result.sdl().type = type;
    return result;
}

// Pushes the events once the demo has initialized video, and exercises the
// demo's window and event loop meanwhile.
auto run_demo(std::initializer_list<sdl::event> events) -> int
{
    std::thread pusher([&] {
        while (sdl::was_init(sdl::init_flags::video) == sdl::init_flags::none)
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
        std::this_thread::sleep_for(std::chrono::milliseconds{50});
        for (sdl::event const& event : events) (void)sdl::push(event);
    });
    int const status = window_demo::run();
    pusher.join();
    return status;
}

} // namespace

int main()
{
    eggs::test_support::use_dummy_drivers();

    EGGS_SDL_CHECK(run_demo({make(SDL_EVENT_QUIT)}) == EXIT_SUCCESS);
    EGGS_SDL_CHECK(
        run_demo({make(SDL_EVENT_WINDOW_CLOSE_REQUESTED)}) == EXIT_SUCCESS
    );
    EGGS_SDL_CHECK(
        run_demo({make_key_down(SDL_SCANCODE_ESCAPE)}) == EXIT_SUCCESS
    );

    // Other keys and pointer motion do not end the loop, only the quit does.
    sdl::event motion = make(SDL_EVENT_MOUSE_MOTION);
    motion.sdl().motion.x = 100.0F;
    motion.sdl().motion.y = 50.0F;
    EGGS_SDL_CHECK(
        run_demo(
            {make_key_down(SDL_SCANCODE_A), motion,
             make(SDL_EVENT_WINDOW_EXPOSED), make(0x7000), make(SDL_EVENT_QUIT)}
        ) == EXIT_SUCCESS
    );

    return eggs::test_support::report();
}
