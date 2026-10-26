// Eggs.SDL
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#include <eggs/sdl.hpp>

#include <chrono>
#include <cstdint>
#include <type_traits>
#include <utility>

#include "support.hpp"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_version.h>

// The event fields are copied from SDL unchanged, so exact comparison is the
// intended check.
#ifdef __clang__
#    pragma clang diagnostic ignored "-Wfloat-equal"
#endif

namespace sdl = eggs::sdl;

namespace {

template <typename... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};

// SDL ends each poll cycle with a sentinel, so a poll can fail while events
// that were pushed since are still queued.
auto poll_cycle(sdl::event& out) -> bool
{
    return sdl::poll(out) || sdl::poll(out);
}

void drain()
{
    sdl::event discarded;
    while (poll_cycle(discarded)) {
    }
}

auto make(std::uint32_t type) -> sdl::event
{
    sdl::event result;
    result.sdl().type = type;
    return result;
}

auto round_trip(sdl::event const& pushed) -> sdl::event
{
    EGGS_SDL_CHECK(sdl::push(pushed).has_value());
    sdl::event polled;
    EGGS_SDL_CHECK(poll_cycle(polled));
    return polled;
}

} // namespace

int main()
{
    eggs::test_support::use_dummy_drivers();

    auto const subsystems = sdl::init(sdl::init_flags::video);
    EGGS_SDL_CHECK(subsystems.has_value());
    if (!subsystems) return eggs::test_support::report();
    drain();

    // -- Polling an empty queue ------------------------------------------------

    sdl::pump();
    EGGS_SDL_CHECK(!sdl::poll().has_value());
    sdl::event scratch;
    EGGS_SDL_CHECK(!sdl::poll(scratch));
    EGGS_SDL_CHECK(!sdl::wait_timeout(scratch, std::chrono::milliseconds{5}));

    // -- Quit ------------------------------------------------------------------

    {
        sdl::event const polled = round_trip(make(SDL_EVENT_QUIT));
        EGGS_SDL_CHECK(polled.type() == sdl::event_type::quit);
        EGGS_SDL_CHECK(polled.holds_alternative<sdl::quit_event>());
        EGGS_SDL_CHECK(!polled.holds_alternative<sdl::unknown_event>());
        EGGS_SDL_CHECK(!polled.holds_alternative<sdl::user_event>());
        auto const quit = polled.get_if<sdl::quit_event>();
        EGGS_SDL_CHECK(quit.has_value());
        EGGS_SDL_CHECK(quit && quit->timestamp() > std::chrono::nanoseconds{0});
        EGGS_SDL_CHECK(!polled.get_if<sdl::key_down_event>().has_value());
        EGGS_SDL_CHECK(
            polled.get<sdl::quit_event>().timestamp() >
            std::chrono::nanoseconds{0}
        );
    }

    // -- Keyboard --------------------------------------------------------------

    {
        sdl::event pushed = make(SDL_EVENT_KEY_DOWN);
        pushed.sdl().key.windowID = 7;
        pushed.sdl().key.which = 3;
        pushed.sdl().key.scancode = SDL_SCANCODE_A;
        pushed.sdl().key.key = SDLK_A;
        pushed.sdl().key.mod = SDL_KMOD_LSHIFT | SDL_KMOD_NUM;
        pushed.sdl().key.raw = 42;
        pushed.sdl().key.down = true;
        pushed.sdl().key.repeat = true;
        sdl::event const polled = round_trip(pushed);

        auto const key = polled.get_if<sdl::key_down_event>();
        EGGS_SDL_CHECK(key.has_value());
        if (key) {
            EGGS_SDL_CHECK(key->window_id() == sdl::window_id{7});
            EGGS_SDL_CHECK(key->which() == sdl::keyboard_id{3});
            EGGS_SDL_CHECK(key->scancode() == sdl::scancode::a);
            EGGS_SDL_CHECK(key->keycode() == sdl::keycode::a);
            EGGS_SDL_CHECK(
                key->modifiers() ==
                (sdl::key_modifiers::left_shift | sdl::key_modifiers::num)
            );
            EGGS_SDL_CHECK(key->raw() == 42);
            EGGS_SDL_CHECK(key->down());
            EGGS_SDL_CHECK(key->repeat());
        }
        EGGS_SDL_CHECK(!polled.holds_alternative<sdl::key_up_event>());
        EGGS_SDL_CHECK(sdl::name(sdl::scancode::escape) == "Escape");
        EGGS_SDL_CHECK(!sdl::name(sdl::scancode::a).empty());
    }

    // -- Mouse -----------------------------------------------------------------

    {
        sdl::event pushed = make(SDL_EVENT_MOUSE_MOTION);
        pushed.sdl().motion.windowID = 2;
        pushed.sdl().motion.which = 5;
        pushed.sdl().motion.state = SDL_BUTTON_LMASK | SDL_BUTTON_RMASK;
        pushed.sdl().motion.x = 12.5F;
        pushed.sdl().motion.y = 34.5F;
        pushed.sdl().motion.xrel = -1.0F;
        pushed.sdl().motion.yrel = 2.0F;
        auto const motion =
            round_trip(pushed).get_if<sdl::mouse_motion_event>();
        EGGS_SDL_CHECK(motion.has_value());
        if (motion) {
            EGGS_SDL_CHECK(motion->window_id() == sdl::window_id{2});
            EGGS_SDL_CHECK(motion->which() == sdl::mouse_id{5});
            EGGS_SDL_CHECK(
                motion->buttons() ==
                (sdl::mouse_button_flags::left | sdl::mouse_button_flags::right)
            );
            EGGS_SDL_CHECK(motion->x() == 12.5F);
            EGGS_SDL_CHECK(motion->y() == 34.5F);
            EGGS_SDL_CHECK(motion->x_relative() == -1.0F);
            EGGS_SDL_CHECK(motion->y_relative() == 2.0F);
        }
    }

    {
        sdl::event pushed = make(SDL_EVENT_MOUSE_BUTTON_UP);
        pushed.sdl().button.windowID = 2;
        pushed.sdl().button.button = SDL_BUTTON_MIDDLE;
        pushed.sdl().button.down = false;
        pushed.sdl().button.clicks = 2;
        pushed.sdl().button.x = 1.0F;
        pushed.sdl().button.y = 2.0F;
        sdl::event const polled = round_trip(pushed);
        EGGS_SDL_CHECK(
            !polled.holds_alternative<sdl::mouse_button_down_event>()
        );
        auto const button = polled.get_if<sdl::mouse_button_up_event>();
        EGGS_SDL_CHECK(button.has_value());
        if (button) {
            EGGS_SDL_CHECK(button->window_id() == sdl::window_id{2});
            EGGS_SDL_CHECK(button->button() == sdl::mouse_button::middle);
            EGGS_SDL_CHECK(!button->down());
            EGGS_SDL_CHECK(button->clicks() == 2);
            EGGS_SDL_CHECK(button->x() == 1.0F);
            EGGS_SDL_CHECK(button->y() == 2.0F);
        }
    }

    {
        sdl::event pushed = make(SDL_EVENT_MOUSE_WHEEL);
        pushed.sdl().wheel.windowID = 2;
        pushed.sdl().wheel.x = 1.5F;
        pushed.sdl().wheel.y = -2.5F;
        pushed.sdl().wheel.direction = SDL_MOUSEWHEEL_FLIPPED;
        auto const wheel = round_trip(pushed).get_if<sdl::mouse_wheel_event>();
        EGGS_SDL_CHECK(wheel.has_value());
        if (wheel) {
            EGGS_SDL_CHECK(wheel->window_id() == sdl::window_id{2});
            EGGS_SDL_CHECK(wheel->x() == 1.5F);
            EGGS_SDL_CHECK(wheel->y() == -2.5F);
            EGGS_SDL_CHECK(
                wheel->direction() == sdl::mouse_wheel_direction::flipped
            );
        }
    }

#if SDL_VERSION_ATLEAST(3, 4, 0)
    {
        sdl::event pushed = make(SDL_EVENT_PINCH_UPDATE);
        pushed.sdl().pinch.windowID = 4;
        pushed.sdl().pinch.scale = 1.25F;
        auto const pinch = round_trip(pushed).get_if<sdl::pinch_update_event>();
        EGGS_SDL_CHECK(pinch.has_value());
        if (pinch) {
            EGGS_SDL_CHECK(pinch->window_id() == sdl::window_id{4});
            EGGS_SDL_CHECK(pinch->scale() == 1.25F);
        }
        EGGS_SDL_CHECK(round_trip(make(SDL_EVENT_PINCH_BEGIN))
                           .holds_alternative<sdl::pinch_begin_event>());
        EGGS_SDL_CHECK(
            round_trip(make(SDL_EVENT_SCREEN_KEYBOARD_SHOWN))
                .holds_alternative<sdl::screen_keyboard_shown_event>()
        );
    }
#endif

    {
        sdl::event pushed = make(SDL_EVENT_DISPLAY_ADDED);
        pushed.sdl().display.displayID = 7;
        auto const added =
            round_trip(pushed).get_if<sdl::display_added_event>();
        EGGS_SDL_CHECK(added.has_value());
        EGGS_SDL_CHECK(added && added->display() == sdl::display_id{7});
    }

    // -- Window ----------------------------------------------------------------

    {
        sdl::event pushed = make(SDL_EVENT_WINDOW_RESIZED);
        pushed.sdl().window.windowID = 9;
        pushed.sdl().window.data1 = 640;
        pushed.sdl().window.data2 = 480;
        sdl::event const polled = round_trip(pushed);
        auto const resized = polled.get_if<sdl::window_resized_event>();
        EGGS_SDL_CHECK(resized.has_value());
        if (resized) {
            EGGS_SDL_CHECK(resized->window_id() == sdl::window_id{9});
            EGGS_SDL_CHECK(resized->size().width == 640);
            EGGS_SDL_CHECK(resized->size().height == 480);
        }
        EGGS_SDL_CHECK(
            !polled.holds_alternative<sdl::window_pixel_size_changed_event>()
        );
    }

    {
        sdl::event pushed = make(SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED);
        pushed.sdl().window.data1 = 1280;
        pushed.sdl().window.data2 = 960;
        auto const changed =
            round_trip(pushed).get_if<sdl::window_pixel_size_changed_event>();
        EGGS_SDL_CHECK(changed && changed->size().width == 1280);
    }

    {
        sdl::event pushed = make(SDL_EVENT_WINDOW_MOVED);
        pushed.sdl().window.data1 = -5;
        pushed.sdl().window.data2 = 15;
        auto const moved = round_trip(pushed).get_if<sdl::window_moved_event>();
        EGGS_SDL_CHECK(moved.has_value());
        EGGS_SDL_CHECK(
            moved && moved->position().x == -5 && moved->position().y == 15
        );
    }

    {
        sdl::event const polled = round_trip(make(SDL_EVENT_WINDOW_EXPOSED));
        EGGS_SDL_CHECK(polled.holds_alternative<sdl::window_exposed_event>());
        sdl::event const closing =
            round_trip(make(SDL_EVENT_WINDOW_CLOSE_REQUESTED));
        EGGS_SDL_CHECK(
            closing.holds_alternative<sdl::window_close_requested_event>()
        );
    }

    // -- User and unknown events -----------------------------------------------

    {
        auto const first = sdl::register_events(2);
        EGGS_SDL_CHECK(first.has_value());
        if (first) {
            int payload = 0;
            sdl::event pushed;
            pushed.sdl().user.type = static_cast<std::uint32_t>(*first) + 1;
            pushed.sdl().user.code = -4;
            pushed.sdl().user.data1 = &payload;
            sdl::event const polled = round_trip(pushed);
            EGGS_SDL_CHECK(polled.holds_alternative<sdl::user_event>());
            EGGS_SDL_CHECK(!polled.holds_alternative<sdl::unknown_event>());
            auto const user = polled.get<sdl::user_event>();
            EGGS_SDL_CHECK(
                user.type() == static_cast<sdl::event_type>(
                                   static_cast<std::uint32_t>(*first) + 1
                               )
            );
            EGGS_SDL_CHECK(user.code() == -4);
            EGGS_SDL_CHECK(user.data1<int>() == &payload);
            EGGS_SDL_CHECK(user.data2() == nullptr);
        }

        sdl::event const polled = round_trip(make(0x7000));
        EGGS_SDL_CHECK(polled.holds_alternative<sdl::unknown_event>());
        EGGS_SDL_CHECK(
            polled.get<sdl::unknown_event>().type() ==
            static_cast<sdl::event_type>(0x7000)
        );
    }

    // -- Visit -----------------------------------------------------------------

    {
        auto const visitor = overloaded{
            [](sdl::quit_event const&) { return 1; },
            [](sdl::key_down_event const&) { return 2; },
            [](sdl::user_event const&) { return 3; },
            [](sdl::unknown_event const&) { return 4; },
            [](auto const&) { return 5; },
        };
        EGGS_SDL_CHECK(make(SDL_EVENT_QUIT).visit(visitor) == 1);
        EGGS_SDL_CHECK(make(SDL_EVENT_KEY_DOWN).visit(visitor) == 2);
        EGGS_SDL_CHECK(make(SDL_EVENT_USER).visit(visitor) == 3);
        EGGS_SDL_CHECK(make(0x7000).visit(visitor) == 4);
        EGGS_SDL_CHECK(make(SDL_EVENT_KEY_UP).visit(visitor) == 5);
        EGGS_SDL_CHECK(make(SDL_EVENT_WINDOW_SHOWN).visit(visitor) == 5);

        make(SDL_EVENT_QUIT).visit([](auto const&) {});
    }

    // -- Wait ------------------------------------------------------------------

    {
        EGGS_SDL_CHECK(sdl::push(make(SDL_EVENT_QUIT)).has_value());
        sdl::event waited;
        EGGS_SDL_CHECK(sdl::wait(waited).has_value());
        EGGS_SDL_CHECK(waited.holds_alternative<sdl::quit_event>());

        EGGS_SDL_CHECK(sdl::push(make(SDL_EVENT_QUIT)).has_value());
        EGGS_SDL_CHECK(
            sdl::wait_timeout(waited, std::chrono::milliseconds{1000})
        );
        EGGS_SDL_CHECK(waited.holds_alternative<sdl::quit_event>());
    }

    // -- Watch -----------------------------------------------------------------

    {
        int calls = 0;
        auto counter = [&](sdl::event const& e) {
            if (e.holds_alternative<sdl::quit_event>()) ++calls;
        };
        {
            auto watch = sdl::add_watch(counter);
            EGGS_SDL_CHECK(watch.has_value());
            EGGS_SDL_CHECK(sdl::push(make(SDL_EVENT_QUIT)).has_value());
            EGGS_SDL_CHECK(calls == 1);

            sdl::event_watch const moved = *std::move(watch);
            EGGS_SDL_CHECK(sdl::push(make(SDL_EVENT_QUIT)).has_value());
            EGGS_SDL_CHECK(calls == 2);
        }
        EGGS_SDL_CHECK(sdl::push(make(SDL_EVENT_QUIT)).has_value());
        EGGS_SDL_CHECK(calls == 2);
        drain();
    }

    // -- Filter ----------------------------------------------------------------

    {
        auto reject_motion = [](sdl::event const& e) {
            return !e.holds_alternative<sdl::mouse_motion_event>();
        };
        {
            sdl::event_filter const filter = sdl::set_filter(reject_motion);
            EGGS_SDL_CHECK(sdl::push(make(SDL_EVENT_QUIT)).has_value());
            auto const rejected = sdl::push(make(SDL_EVENT_MOUSE_MOTION));
            EGGS_SDL_CHECK(!rejected.has_value());
            EGGS_SDL_CHECK(
                !rejected.has_value() && rejected.error().message().empty()
            );
            drain();
        }
        EGGS_SDL_CHECK(sdl::push(make(SDL_EVENT_MOUSE_MOTION)).has_value());
        drain();

        // A replaced filter is not cleared by the one that replaced it.
        auto keep_all = [](sdl::event const&) {
            return true;
        };
        {
            sdl::event_filter first = sdl::set_filter(reject_motion);
            sdl::event_filter const second = sdl::set_filter(keep_all);
            first = {};
            EGGS_SDL_CHECK(sdl::push(make(SDL_EVENT_MOUSE_MOTION)).has_value());
            drain();
        }
    }

    return eggs::test_support::report();
}
