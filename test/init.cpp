// Eggs.SDL
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#include <eggs/sdl.hpp>

#include <utility>

#include "support.hpp"

#include <SDL3/SDL_init.h>

namespace sdl = eggs::sdl;

int main()
{
    eggs::test_support::use_dummy_drivers();

    EGGS_SDL_CHECK(sdl::was_init() == sdl::init_flags::none);

    // -- Init and quit ---------------------------------------------------------

    {
        auto first = sdl::init(sdl::init_flags::video);
        EGGS_SDL_CHECK(first.has_value());
        if (!first) return eggs::test_support::report();

        EGGS_SDL_CHECK(*first);
        EGGS_SDL_CHECK((*first)->flags() == sdl::init_flags::video);
        EGGS_SDL_CHECK(
            sdl::was_init(sdl::init_flags::video) == sdl::init_flags::video
        );
        EGGS_SDL_CHECK(
            sdl::was_init(sdl::init_flags::events) == sdl::init_flags::events
        );
        EGGS_SDL_CHECK(
            sdl::was_init(sdl::init_flags::audio) == sdl::init_flags::none
        );

        // -- Nested ownership --------------------------------------------------

        {
            auto second = sdl::init(sdl::init_flags::video);
            EGGS_SDL_CHECK(second.has_value());
        }
        EGGS_SDL_CHECK(
            sdl::was_init(sdl::init_flags::video) == sdl::init_flags::video
        );

        // -- Moves -------------------------------------------------------------

        sdl::unique_subsystems owner = *std::move(first);
        EGGS_SDL_CHECK(owner);
        sdl::unique_subsystems const moved = std::move(owner);
        EGGS_SDL_CHECK(!owner);
        EGGS_SDL_CHECK(moved);
    }
    EGGS_SDL_CHECK(sdl::was_init() == sdl::init_flags::none);

    // -- Owners only quit what they initialized --------------------------------

    {
        auto events = sdl::init(sdl::init_flags::events);
        EGGS_SDL_CHECK(events.has_value());
        {
            auto video = sdl::init(sdl::init_flags::video);
            EGGS_SDL_CHECK(video.has_value());
        }
        EGGS_SDL_CHECK(
            sdl::was_init(sdl::init_flags::events) == sdl::init_flags::events
        );
        EGGS_SDL_CHECK(
            sdl::was_init(sdl::init_flags::video) == sdl::init_flags::none
        );
    }
    EGGS_SDL_CHECK(sdl::was_init() == sdl::init_flags::none);

    // -- Explicit quit ---------------------------------------------------------

    {
        auto video = sdl::init(sdl::init_flags::video);
        EGGS_SDL_CHECK(video.has_value());
        sdl::quit();
        EGGS_SDL_CHECK(sdl::was_init() == sdl::init_flags::none);
    }

    // -- App metadata ----------------------------------------------------------

    EGGS_SDL_CHECK(
        sdl::set_app_metadata("Eggs.SDL", "1.0.0", "org.eggs.sdl").has_value()
    );
    EGGS_SDL_CHECK(
        sdl::set_app_metadata_property(
            SDL_PROP_APP_METADATA_URL_STRING, "https://example.com"
        )
            .has_value()
    );

    auto const invalid = sdl::set_app_metadata_property("", "value");
    EGGS_SDL_CHECK(!invalid.has_value());
    EGGS_SDL_CHECK(!invalid.error().message().empty());

    return eggs::test_support::report();
}
