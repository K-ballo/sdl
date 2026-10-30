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
#include <format>
#include <optional>
#include <print>

// The demo's texture, drawing and event loop, so that the example executable
// and the test that drives it share one definition.
namespace render_demo {

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

// What the demo draws in, regardless of the size of the window.
constexpr sdl::extent logical_size = {.width = 320, .height = 180};

// Draws a 16 by 16 checkerboard of 4 pixel cells.
inline auto draw_checkerboard(sdl::surface board)
    -> std::expected<void, sdl::error>
{
    constexpr int size = 16;
    constexpr int cell = 4;
    constexpr sdl::rgba light = {.r = 240, .g = 200, .b = 60};
    constexpr sdl::rgba dark = {.r = 200, .g = 60, .b = 60};

    for (int y = 0; y != size / cell; ++y) {
        for (int x = 0; x != size / cell; ++x) {
            auto const filled = board.fill(
                {.x = x * cell, .y = y * cell, .width = cell, .height = cell},
                (x + y) % 2 == 0 ? light : dark
            );
            if (!filled) return filled;
        }
    }
    return {};
}

inline auto make_checkerboard()
    -> std::expected<sdl::unique_surface, sdl::error>
{
    auto created = sdl::surface::create(
        {.width = 16, .height = 16}, sdl::pixel_format::rgba32
    );
    if (created) {
        if (auto const drawn = draw_checkerboard(created->get()); !drawn)
            created = std::unexpected(drawn.error());
    }
    return created;
}

// Where something is along an axis, given how far it has gone in all, and
// turning around at the ends: the room is how far it can be.
inline auto bounce(float distance, float room) -> float
{
    if (room <= 0.0F) return 0.0F;
    float const along = std::fmod(distance, 2.0F * room);
    return along <= room ? along : (2.0F * room) - along;
}

// What changes from one frame to the next.
struct frame_state
{
    // Since the demo started.
    std::chrono::nanoseconds elapsed;
    float frames_per_second;
    // In logical coordinates, once the pointer has moved over the window.
    std::optional<sdl::fpoint> cursor;
};

// Draws a frame in full and presents it. The screenshot is taken before
// present, the back buffer is undefined after it.
inline auto draw(
    sdl::renderer renderer, sdl::texture sprite, frame_state const& state,
    char const* screenshot
) -> std::expected<void, sdl::error>
{
    constexpr float side = 32.0F;
    float const seconds = std::chrono::duration<float>(state.elapsed).count();

    constexpr auto width = static_cast<float>(logical_size.width);
    constexpr auto height = static_cast<float>(logical_size.height);

    auto drawn = renderer.set_draw_color(sdl::rgba{.r = 20, .g = 24, .b = 40})
                     .and_then([&] { return renderer.clear(); });

    // The sprite bounces about the logical area, and a copy rotates about its
    // own center.
    drawn = drawn.and_then([&] {
        return renderer.draw_texture(
            sprite, {},
            sdl::frect{
                .x = bounce(seconds * 90.0F, width - side),
                .y = bounce(seconds * 60.0F, height - side),
                .width = side,
                .height = side
            }
        );
    });
    drawn = drawn.and_then([&] {
        return renderer.draw_texture_rotated(
            sprite, {},
            sdl::frect{
                .x = (width - side) / 2.0F,
                .y = (height - side) / 2.0F,
                .width = side,
                .height = side
            },
            static_cast<double>(seconds) * 90.0
        );
    });

    // A filled and an outlined rectangle.
    drawn = drawn.and_then([&] {
        return renderer.set_draw_color(sdl::rgba{.r = 60, .g = 160, .b = 90});
    });
    drawn = drawn.and_then([&] {
        return renderer.fill_rect(
            {.x = 12.0F, .y = 12.0F, .width = 60.0F, .height = 30.0F}
        );
    });
    drawn = drawn.and_then([&] {
        return renderer.set_draw_color(sdl::rgba{.r = 230, .g = 230, .b = 230});
    });
    drawn = drawn.and_then([&] {
        return renderer.draw_rect(
            {.x = 80.0F, .y = 12.0F, .width = 60.0F, .height = 30.0F}
        );
    });

    // A connected polyline.
    std::array<sdl::fpoint, 5> const zigzag = {
        sdl::fpoint{.x = 12.0F, .y = 150.0F},
        sdl::fpoint{.x = 40.0F, .y = 120.0F},
        sdl::fpoint{.x = 68.0F, .y = 150.0F},
        sdl::fpoint{.x = 96.0F, .y = 120.0F},
        sdl::fpoint{.x = 124.0F, .y = 150.0F}
    };
    drawn = drawn.and_then([&] { return renderer.draw_lines(zigzag); });

    // A triangle with a color at each corner and no texture.
    std::array<sdl::vertex, 3> const triangle = {
        sdl::vertex{
            .position = {.x = 250.0F, .y = 100.0F},
            .color = {.r = 1.0F, .g = 0.0F, .b = 0.0F},
            .tex_coord = {}
        },
        sdl::vertex{
            .position = {.x = 220.0F, .y = 160.0F},
            .color = {.r = 0.0F, .g = 1.0F, .b = 0.0F},
            .tex_coord = {}
        },
        sdl::vertex{
            .position = {.x = 280.0F, .y = 160.0F},
            .color = {.r = 0.0F, .g = 0.0F, .b = 1.0F},
            .tex_coord = {}
        }
    };
    drawn = drawn.and_then([&] {
        return renderer.draw_geometry(sdl::texture{}, triangle);
    });

    // The crosshair is where the pointer is, in logical coordinates.
    if (state.cursor) {
        sdl::fpoint const at = *state.cursor;
        constexpr float arm = 5.0F;
        drawn = drawn.and_then([&] {
            return renderer.set_draw_color(
                sdl::rgba{.r = 255, .g = 255, .b = 0}
            );
        });
        drawn = drawn.and_then([&] {
            return renderer.draw_line(
                {.x = at.x - arm, .y = at.y}, {.x = at.x + arm, .y = at.y}
            );
        });
        drawn = drawn.and_then([&] {
            return renderer.draw_line(
                {.x = at.x, .y = at.y - arm}, {.x = at.x, .y = at.y + arm}
            );
        });
    }

    drawn = drawn.and_then([&] {
        return renderer.set_draw_color(sdl::rgba{.r = 255, .g = 255, .b = 255});
    });
    drawn = drawn.and_then([&] {
        auto const text = std::format("{:.0f} fps", state.frames_per_second);
        return renderer.draw_debug_text(
            {.x = 150.0F, .y = 12.0F}, text.c_str()
        );
    });

    if (screenshot != nullptr) {
        drawn = drawn.and_then([&] {
            return renderer.read_pixels().and_then(
                [&](sdl::unique_surface const& pixels) {
                    return pixels->save_bmp(screenshot);
                }
            );
        });
    }

    return drawn.and_then([&] { return renderer.present(); });
}

struct options
{
    // A BMP file to load the sprite from, drawn when there is none.
    char const* sprite = nullptr;
    // Where the S key saves the screenshot.
    char const* screenshot = "screenshot.bmp";
};

// Runs until the window is closed.
inline auto run(options const& options = {}) -> int
{
    auto const subsystems = sdl::init(sdl::init_flags::video);
    if (!subsystems) return fail(subsystems.error());

    auto const owned = sdl::window::create(
        "Eggs.SDL render demo", 800, 600,
        sdl::window_flags::resizable | sdl::window_flags::high_pixel_density
    );
    if (!owned) return fail(owned.error());
    sdl::window const window = owned->get();

    // The renderer is destroyed after the textures that are made from it,
    // which are declared below it.
    auto const created = sdl::renderer::create(window);
    if (!created) return fail(created.error());
    sdl::renderer const renderer = created->get();

    if (auto const name = renderer.name()) std::print("renderer: {}\n", *name);

    sdl::logical_presentation_mode mode =
        sdl::logical_presentation_mode::letterbox;
    if (auto const set = renderer.set_vsync(sdl::vsync_mode::every_refresh);
        !set)
        return fail(set.error());
    if (auto const set = renderer.set_logical_presentation(logical_size, mode);
        !set)
        return fail(set.error());

    auto loaded = options.sprite != nullptr
                      ? sdl::surface::load_bmp(options.sprite)
                      : make_checkerboard();
    if (!loaded) return fail(loaded.error());

    // The surface is copied and can go, but it is needed for no more.
    auto const made = sdl::texture::from_surface(renderer, loaded->get());
    if (!made) return fail(made.error());
    sdl::texture const sprite = made->get();
    loaded->reset();

    // Crisp pixels when the sprite is scaled up.
    if (auto const set = sprite.set_scale_mode(sdl::scale_mode::nearest); !set)
        return fail(set.error());

    bool running = true;
    bool screenshot = false;
    std::expected<void, sdl::error> result;
    frame_state state = {
        .elapsed = {}, .frames_per_second = 0.0F, .cursor = std::nullopt
    };
    auto const start = sdl::clock::now();
    auto last = start;
    sdl::event event;
    while (running && result) {
        auto const frame_start = sdl::clock::now();
        state.elapsed = frame_start - start;

        // The frames per second, smoothed over about ten frames.
        float const delta =
            std::chrono::duration<float>(frame_start - last).count();
        last = frame_start;
        if (delta > 0.0F) {
            float const instant = 1.0F / delta;
            state.frames_per_second =
                state.frames_per_second == 0.0F
                    ? instant
                    : state.frames_per_second +
                          ((instant - state.frames_per_second) * 0.1F);
        }

        while (sdl::poll(event)) {
            // The alternatives are extracted after the conversion, so that
            // they carry logical coordinates.
            if (event.holds_alternative<sdl::mouse_motion_event>()) {
                result = renderer.convert_event_coordinates(event);
                if (!result) break;
            }
            event.visit(
                overloaded{
                    [&](sdl::quit_event const&) { running = false; },
                    [&](sdl::window_close_requested_event const&) {
                        running = false;
                    },
                    [&](sdl::key_down_event const& key) {
                        if (key.scancode() == sdl::scancode::escape) {
                            running = false;
                        } else if (key.scancode() == sdl::scancode::space) {
                            mode =
                                mode == sdl::logical_presentation_mode::
                                            letterbox
                                    ? sdl::logical_presentation_mode::
                                          integer_scale
                                    : sdl::logical_presentation_mode::letterbox;
                            result = renderer.set_logical_presentation(
                                logical_size, mode
                            );
                        } else if (key.scancode() == sdl::scancode::s) {
                            screenshot = true;
                        }
                    },
                    [&](sdl::mouse_motion_event const& motion) {
                        state.cursor =
                            sdl::fpoint{.x = motion.x(), .y = motion.y()};
                    },
                    [](auto const&) {},
                }
            );
        }
        if (!result) break;

        // A screenshot that was asked for is taken even if the demo is
        // closing, the frame is drawn for that.
        if (!running && !screenshot) break;
        result = draw(
            renderer, sprite, state, screenshot ? options.screenshot : nullptr
        );
        screenshot = false;
    }

    if (!result) return fail(result.error());
    return EXIT_SUCCESS;
}

} // namespace render_demo
