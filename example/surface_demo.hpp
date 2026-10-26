// Eggs.SDL
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <eggs/sdl.hpp>

#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <expected>
#include <print>

// The demo's sprite, drawing and event loop, so that the example executable
// and the test that drives it share one definition.
namespace surface_demo {

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

// Draws a checkerboard with a border over the whole sprite, which is 64 by 64.
inline auto draw_pattern(sdl::surface sprite) -> std::expected<void, sdl::error>
{
    constexpr int size = 64;
    constexpr int cell = 8;
    constexpr sdl::rgba light = {.r = 240, .g = 200, .b = 60};
    constexpr sdl::rgba dark = {.r = 200, .g = 60, .b = 60};
    constexpr sdl::rgba border = {.r = 20, .g = 20, .b = 20};

    for (int y = 0; y != size / cell; ++y) {
        for (int x = 0; x != size / cell; ++x) {
            auto const filled = sprite.fill(
                {.x = x * cell, .y = y * cell, .width = cell, .height = cell},
                (x + y) % 2 == 0 ? light : dark
            );
            if (!filled) return filled;
        }
    }

    std::array<sdl::rect, 4> const edges = {
        sdl::rect{.x = 0, .y = 0, .width = size, .height = 2},
        sdl::rect{.x = 0, .y = size - 2, .width = size, .height = 2},
        sdl::rect{.x = 0, .y = 0, .width = 2, .height = size},
        sdl::rect{.x = size - 2, .y = 0, .width = 2, .height = size}
    };
    return sprite.fill(edges, border);
}

inline auto make_pattern() -> std::expected<sdl::unique_surface, sdl::error>
{
    auto created = sdl::surface::create(
        {.width = 64, .height = 64}, sdl::pixel_format::rgba32
    );
    if (created) {
        if (auto const drawn = draw_pattern(created->get()); !drawn)
            created = std::unexpected(drawn.error());
    }
    return created;
}

// Where a sprite is along an axis, given how far it has gone in all, and
// turning around at the ends: the room is how far it can be.
inline auto bounce(float distance, int room) -> int
{
    if (room <= 0) return 0;
    auto const length = static_cast<float>(room);
    float const along = std::fmod(distance, 2.0F * length);
    return static_cast<int>(along <= length ? along : (2.0F * length) - along);
}

// The surface of the window is re-acquired on every frame, it is invalidated
// by resizes.
inline auto
draw(sdl::window window, sdl::surface sprite, sdl::clock::time_point now)
    -> std::expected<void, sdl::error>
{
    auto const acquired = window.get_surface();
    if (!acquired) return std::unexpected(acquired.error());
    sdl::surface const target = *acquired;

    if (auto const cleared = target.clear({.r = 0.1F, .g = 0.1F, .b = 0.2F});
        !cleared)
        return cleared;

    float const seconds =
        std::chrono::duration<float>(now.time_since_epoch()).count();
    sdl::extent const room = {
        .width = target.size().width - sprite.size().width,
        .height = target.size().height - sprite.size().height
    };
    sdl::point const at = {
        .x = bounce(seconds * 220.0F, room.width),
        .y = bounce(seconds * 150.0F, room.height)
    };
    if (auto const blitted = sprite.blit(target, at); !blitted) return blitted;

    // Once more, twice the size, in the middle.
    sdl::extent const size = sprite.size();
    sdl::rect const middle = {
        .x = (target.size().width - (2 * size.width)) / 2,
        .y = (target.size().height - (2 * size.height)) / 2,
        .width = 2 * size.width,
        .height = 2 * size.height
    };
    if (auto const scaled =
            sprite.blit_scaled(target, middle, sdl::scale_mode::nearest);
        !scaled)
        return scaled;

    return window.update_surface();
}

struct options
{
    // A BMP file to load the sprite from, drawn when there is none.
    char const* sprite = nullptr;
    // Where the S key saves the window.
    char const* frame = "frame.bmp";
};

// Runs until the window is closed.
inline auto run(options const& options = {}) -> int
{
    auto const subsystems = sdl::init(sdl::init_flags::video);
    if (!subsystems) return fail(subsystems.error());

    auto const owned = sdl::window::create(
        "Eggs.SDL surface demo", 800, 600, sdl::window_flags::resizable
    );
    if (!owned) return fail(owned.error());
    sdl::window const window = owned->get();

    auto const first = window.get_surface();
    if (!first) return fail(first.error());
    sdl::pixel_format const format = first->format();
    std::print("window surface format: {}\n", sdl::name(format));

    auto loaded = options.sprite != nullptr
                      ? sdl::surface::load_bmp(options.sprite)
                      : make_pattern();
    if (!loaded) return fail(loaded.error());

    // Converted once, to the format that it is blitted to.
    auto converted = (*loaded)->convert(format);
    if (!converted) return fail(converted.error());
    sdl::surface const sprite = converted->get();

    bool running = true;
    std::expected<void, sdl::error> result;
    sdl::event event;
    while (running && result) {
        auto const frame_start = sdl::clock::now();

        while (sdl::poll(event)) {
            event.visit(
                overloaded{
                    [&](sdl::quit_event const&) { running = false; },
                    [&](sdl::window_close_requested_event const&) {
                        running = false;
                    },
                    [&](sdl::key_down_event const& key) {
                        if (key.scancode() == sdl::scancode::escape)
                            running = false;
                        else if (key.scancode() == sdl::scancode::s)
                            result = window.get_surface().and_then(
                                [&](sdl::surface saved) {
                                    return saved.save_bmp(options.frame);
                                }
                            );
                    },
                    [](auto const&) {},
                }
            );
        }
        if (!running || !result) break;

        result = draw(window, sprite, frame_start);

        // About 60 frames per second.
        constexpr std::chrono::nanoseconds frame{1'000'000'000 / 60};
        sdl::delay(frame - (sdl::clock::now() - frame_start));
    }

    if (!result) return fail(result.error());
    return EXIT_SUCCESS;
}

} // namespace surface_demo
