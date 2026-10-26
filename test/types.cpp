// Eggs.SDL
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#include <eggs/sdl.hpp>

#include <cstdint>
#include <memory>
#include <type_traits>

#include "support.hpp"

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_version.h>
#include <SDL3/SDL_video.h>

namespace sdl = eggs::sdl;

// -- Handles and owners ------------------------------------------------------

static_assert(std::is_trivially_copyable_v<sdl::window>);
static_assert(std::is_trivially_copyable_v<sdl::surface>);
static_assert(std::is_trivially_copyable_v<sdl::subsystems>);
static_assert(sizeof(sdl::window) == sizeof(void*));
static_assert(sizeof(sdl::surface) == sizeof(void*));
static_assert(sizeof(sdl::subsystems) == sizeof(void*));
static_assert(sizeof(sdl::unique_window) == sizeof(void*));
static_assert(sizeof(sdl::unique_subsystems) == sizeof(void*));
static_assert(std::is_same_v<sdl::window_deleter::pointer, sdl::window>);
static_assert(
    std::is_same_v<sdl::subsystems_deleter::pointer, sdl::subsystems>
);
static_assert(std::is_nothrow_default_constructible_v<sdl::window>);
static_assert(!std::is_copy_constructible_v<sdl::unique_window>);
static_assert(std::is_nothrow_move_constructible_v<sdl::unique_window>);
static_assert(std::is_nothrow_move_constructible_v<sdl::event_watch>);
static_assert(std::is_nothrow_move_constructible_v<sdl::event_filter>);
static_assert(!std::is_copy_constructible_v<sdl::event_watch>);
static_assert(!std::is_copy_constructible_v<sdl::event_filter>);

// -- Strong ids --------------------------------------------------------------

static_assert(!std::is_convertible_v<std::uint32_t, sdl::window_id>);
static_assert(!std::is_convertible_v<sdl::window_id, std::uint32_t>);
static_assert(!std::is_same_v<sdl::window_id, sdl::keyboard_id>);
static_assert(!std::is_same_v<sdl::keyboard_id, sdl::mouse_id>);

// -- Events ------------------------------------------------------------------

static_assert(std::is_trivially_copyable_v<sdl::event>);
static_assert(std::is_trivially_copyable_v<sdl::unknown_event>);
static_assert(std::is_trivially_copyable_v<sdl::user_event>);

#define EGGS_SDL_CHECK_ALTERNATIVE(name, value, family)                 \
    static_assert(std::is_trivially_copyable_v<sdl::name##_event>);     \
    static_assert(                                                      \
        static_cast<std::uint32_t>(sdl::event_type::name) == (value)    \
    );                                                                  \
    static_assert(sdl::name##_event::type == sdl::event_type::name);    \
    static_assert(sdl::name##_event::matches(sdl::event_type::name));   \
    static_assert(!sdl::unknown_event::matches(sdl::event_type::name)); \
    static_assert(!sdl::user_event::matches(sdl::event_type::name));
EGGS_SDL_EVENT_TYPES(EGGS_SDL_CHECK_ALTERNATIVE)
#undef EGGS_SDL_CHECK_ALTERNATIVE

static_assert(sdl::unknown_event::matches(sdl::event_type::private0));
static_assert(sdl::unknown_event::matches(sdl::event_type::poll_sentinel));
static_assert(!sdl::unknown_event::matches(sdl::event_type::user));
static_assert(sdl::user_event::matches(sdl::event_type::user));
static_assert(sdl::user_event::matches(sdl::event_type::last));

// -- Enumerations carry SDL values -------------------------------------------

static_assert(
    static_cast<std::uint32_t>(sdl::init_flags::video) == SDL_INIT_VIDEO
);
static_assert(
    static_cast<std::uint32_t>(sdl::init_flags::camera) == SDL_INIT_CAMERA
);
static_assert(
    static_cast<std::uint64_t>(sdl::window_flags::resizable) ==
    SDL_WINDOW_RESIZABLE
);
static_assert(
    static_cast<std::uint64_t>(sdl::window_flags::not_focusable) ==
    SDL_WINDOW_NOT_FOCUSABLE
);
static_assert(
    static_cast<std::uint32_t>(sdl::scancode::escape) == SDL_SCANCODE_ESCAPE
);
#if SDL_VERSION_ATLEAST(3, 4, 0)
static_assert(
    static_cast<std::uint64_t>(sdl::window_flags::fill_document) ==
    SDL_WINDOW_FILL_DOCUMENT
);
static_assert(
    static_cast<std::uint32_t>(sdl::scancode::front) == SDL_SCANCODE_FRONT
);
static_assert(
    static_cast<std::int32_t>(sdl::progress_state::error) ==
    SDL_PROGRESS_STATE_ERROR
);
#endif
static_assert(static_cast<std::uint32_t>(sdl::keycode::escape) == SDLK_ESCAPE);
static_assert(
    static_cast<std::uint16_t>(sdl::key_modifiers::ctrl) == SDL_KMOD_CTRL
);
static_assert(
    static_cast<std::uint8_t>(sdl::mouse_button::x2) == SDL_BUTTON_X2
);

int main()
{
    // -- Flag operators --------------------------------------------------------

    auto flags = sdl::init_flags::video | sdl::init_flags::audio;
    EGGS_SDL_CHECK(
        static_cast<std::uint32_t>(flags) == (SDL_INIT_VIDEO | SDL_INIT_AUDIO)
    );
    EGGS_SDL_CHECK((flags & sdl::init_flags::video) == sdl::init_flags::video);
    EGGS_SDL_CHECK((flags & sdl::init_flags::camera) == sdl::init_flags::none);
    flags &= ~sdl::init_flags::audio;
    EGGS_SDL_CHECK(flags == sdl::init_flags::video);
    flags |= sdl::init_flags::events;
    EGGS_SDL_CHECK((flags ^ sdl::init_flags::video) == sdl::init_flags::events);
    flags ^= sdl::init_flags::events;
    EGGS_SDL_CHECK(flags == sdl::init_flags::video);

    auto const window =
        sdl::window_flags::resizable | sdl::window_flags::hidden;
    EGGS_SDL_CHECK(
        static_cast<std::uint64_t>(window) ==
        (SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN)
    );

    // -- Null handles ----------------------------------------------------------

    sdl::window const null_window;
    EGGS_SDL_CHECK(!null_window);
    EGGS_SDL_CHECK(null_window.handle() == nullptr);
    EGGS_SDL_CHECK(null_window == sdl::window{});
    EGGS_SDL_CHECK(null_window == nullptr);

    sdl::surface const null_surface;
    EGGS_SDL_CHECK(!null_surface);
    EGGS_SDL_CHECK(null_surface.handle() == nullptr);

    sdl::unique_window const owner;
    EGGS_SDL_CHECK(!owner);
    EGGS_SDL_CHECK(owner.get() == null_window);

    sdl::unique_subsystems const subsystems;
    EGGS_SDL_CHECK(!subsystems);
    EGGS_SDL_CHECK(subsystems.get().flags() == sdl::init_flags::none);

    // -- Default events --------------------------------------------------------

    sdl::event const event;
    EGGS_SDL_CHECK(event.type() == sdl::event_type::first);
    EGGS_SDL_CHECK(event.holds_alternative<sdl::unknown_event>());
    EGGS_SDL_CHECK(!event.holds_alternative<sdl::quit_event>());
    EGGS_SDL_CHECK(!event.holds_alternative<sdl::user_event>());
    EGGS_SDL_CHECK(!event.get_if<sdl::quit_event>().has_value());

    return eggs::test_support::report();
}
