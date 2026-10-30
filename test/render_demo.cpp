// Eggs.SDL
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#include "render_demo.hpp"

#include <eggs/sdl.hpp>

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <initializer_list>
#include <string>
#include <thread>

#include "support.hpp"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_thread.h>

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

auto make_motion(float x, float y) -> sdl::event
{
    sdl::event result;
    result.sdl().type = SDL_EVENT_MOUSE_MOTION;
    result.sdl().motion.x = x;
    result.sdl().motion.y = y;
    return result;
}

auto make(std::uint32_t type) -> sdl::event
{
    sdl::event result;
    result.sdl().type = type;
    return result;
}

// Pushes the events once the demo has initialized video, and exercises the
// demo's drawing and event loop meanwhile.
auto run_demo(
    std::initializer_list<sdl::event> events,
    render_demo::options const& options = {}
) -> int
{
    std::thread pusher([&] {
        while (sdl::was_init(sdl::init_flags::video) == sdl::init_flags::none)
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
        std::this_thread::sleep_for(std::chrono::milliseconds{100});
        for (sdl::event const& event : events) (void)sdl::push(event);
        SDL_CleanupTLS();
    });
    int const status = render_demo::run(options);
    pusher.join();
    return status;
}

} // namespace

int main()
{
    eggs::test_support::use_dummy_drivers();

    // -- The bounce stays within the room --------------------------------------

    EGGS_SDL_CHECK(render_demo::bounce(0.0F, 100.0F) == 0.0F);
    EGGS_SDL_CHECK(render_demo::bounce(60.0F, 100.0F) == 60.0F);
    EGGS_SDL_CHECK(render_demo::bounce(140.0F, 100.0F) == 60.0F);
    EGGS_SDL_CHECK(render_demo::bounce(200.0F, 100.0F) == 0.0F);
    EGGS_SDL_CHECK(render_demo::bounce(50.0F, 0.0F) == 0.0F);

    // -- Every way out ---------------------------------------------------------

    EGGS_SDL_CHECK(run_demo({make(SDL_EVENT_QUIT)}) == EXIT_SUCCESS);
    EGGS_SDL_CHECK(
        run_demo({make(SDL_EVENT_WINDOW_CLOSE_REQUESTED)}) == EXIT_SUCCESS
    );
    EGGS_SDL_CHECK(
        run_demo({make_key_down(SDL_SCANCODE_ESCAPE)}) == EXIT_SUCCESS
    );

    // -- The pointer, the presentation mode, a resize --------------------------

    EGGS_SDL_CHECK(
        run_demo(
            {make_motion(400.0F, 300.0F), make_key_down(SDL_SCANCODE_SPACE),
             make(SDL_EVENT_WINDOW_RESIZED), make_key_down(SDL_SCANCODE_SPACE),
             make_key_down(SDL_SCANCODE_A), make(SDL_EVENT_QUIT)}
        ) == EXIT_SUCCESS
    );

    // -- S saves a screenshot --------------------------------------------------

    auto const screenshot =
        (std::filesystem::temp_directory_path() / "eggs_sdl_screenshot.bmp")
            .string();
    (void)std::remove(screenshot.c_str());
    EGGS_SDL_CHECK(
        run_demo(
            {make_key_down(SDL_SCANCODE_S), make(SDL_EVENT_QUIT)},
            {.screenshot = screenshot.c_str()}
        ) == EXIT_SUCCESS
    );
    auto const saved = sdl::surface::load_bmp(screenshot.c_str());
    EGGS_SDL_CHECK(saved.has_value());
    if (saved) {
        EGGS_SDL_CHECK((*saved)->size().width == 800);
        // The logical area of 16 by 9 within the 4 by 3 window: the bars of the
        // letterbox are not read.
        EGGS_SDL_CHECK((*saved)->size().height == 450);
    }

    // -- A sprite from a file --------------------------------------------------

    EGGS_SDL_CHECK(
        run_demo({make(SDL_EVENT_QUIT)}, {.sprite = screenshot.c_str()}) ==
        EXIT_SUCCESS
    );
    (void)std::remove(screenshot.c_str());

    // -- An error ends the demo with a failure ---------------------------------

    // No events are pushed, it fails before the loop.
    EGGS_SDL_CHECK(
        render_demo::run({.sprite = "no/such/sprite.bmp"}) == EXIT_FAILURE
    );

    return eggs::test_support::report();
}
