// Eggs.SDL
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <eggs/sdl.hpp>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <expected>
#include <print>

// The demo's window and event loop, so that the example executable and the
// test that drives it share one definition.
namespace window_demo {

namespace sdl = eggs::sdl;

template <typename... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};

inline auto fail(sdl::error const& error) -> int
{
    std::print(stderr, "error: {}\n", error.message());
    return EXIT_FAILURE;
}

inline auto channel(float position, int extent) -> std::uint8_t
{
    float const scaled = position * 255.0F / static_cast<float>(extent);
    return static_cast<std::uint8_t>(std::clamp(scaled, 0.0F, 255.0F));
}

// The surface is re-acquired on every paint, it is invalidated by resizes.
inline auto paint(sdl::window window, sdl::rgb color)
    -> std::expected<void, sdl::error>
{
    return window.get_surface()
        .and_then([&](sdl::surface surface) { return surface.fill(color); })
        .and_then([&] { return window.update_surface(); });
}

inline auto run() -> int
{
    auto const subsystems = sdl::init(sdl::init_flags::video);
    if (!subsystems) return fail(subsystems.error());

    if (auto const metadata = sdl::set_app_metadata(
            "Eggs.SDL window demo", "1.0.0", "org.eggs.sdl.window_demo"
        );
        !metadata)
        return fail(metadata.error());

    auto const owned = sdl::window::create(
        "Eggs.SDL window demo", 800, 600, sdl::window_flags::resizable
    );
    if (!owned) return fail(owned.error());
    sdl::window const window = owned->get();

    sdl::rgb color = {.r = 0, .g = 0, .b = 255};
    bool running = true;
    std::expected<void, sdl::error> result;

    sdl::event event;
    while (running) {
        if (auto const waited = sdl::wait(event); !waited)
            return fail(waited.error());

        event.visit(
            overloaded{
                [&](sdl::quit_event const&) { running = false; },
                [&](sdl::window_close_requested_event const&) {
                    running = false;
                },
                [&](sdl::key_down_event const& key) {
                    auto const name = sdl::name(key.scancode());
                    std::print(
                        "{} (modifiers: 0x{:04x})\n", name,
                        static_cast<unsigned>(key.modifiers())
                    );
                    if (key.scancode() == sdl::scancode::escape)
                        running = false;
                },
                [&](sdl::mouse_motion_event const& motion) {
                    auto const size = window.size();
                    if (!size) {
                        result = std::unexpected(size.error());
                        return;
                    }
                    color.r = channel(motion.x(), size->width);
                    color.g = channel(motion.y(), size->height);
                    result = paint(window, color);
                },
                [&](sdl::window_resized_event const&) {
                    result = paint(window, color);
                },
                [&](sdl::window_pixel_size_changed_event const&) {
                    result = paint(window, color);
                },
                [&](sdl::window_exposed_event const&) {
                    result = paint(window, color);
                },
                [](auto const&) {},
            }
        );

        if (!result) return fail(result.error());
    }
    return EXIT_SUCCESS;
}

} // namespace window_demo
