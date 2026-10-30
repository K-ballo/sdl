// Eggs.SDL
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#include <eggs/sdl.hpp>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "support.hpp"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_video.h>

namespace sdl = eggs::sdl;

// The overloads of set_draw_color are told apart by the name of the type, a
// braced list could be either.
static_assert(requires(sdl::renderer r) { r.set_draw_color(sdl::rgba{}); });
static_assert(requires(sdl::renderer r) { r.set_draw_color(sdl::fcolor{}); });
static_assert(requires(sdl::renderer r) { r.set_draw_color(sdl::rgb{}); });
template <typename R>
concept accepts_braces = requires(R r) { r.set_draw_color({}); };

static_assert(!accepts_braces<sdl::renderer>);

namespace {

constexpr sdl::rgba black = {.r = 0, .g = 0, .b = 0};
constexpr sdl::rgba red = {.r = 255, .g = 0, .b = 0};
constexpr sdl::rgba green = {.r = 0, .g = 255, .b = 0};
constexpr sdl::rgba white = {.r = 255, .g = 255, .b = 255};

auto same(sdl::rgba a, sdl::rgba b) -> bool
{
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

auto near(sdl::fpoint a, sdl::fpoint b) -> bool
{
    return std::fabs(a.x - b.x) < 0.001F && std::fabs(a.y - b.y) < 0.001F;
}

// The color of a pixel of what the renderer drew, black on error.
auto pixel(sdl::renderer renderer, int x, int y) -> sdl::rgba
{
    auto const read = renderer.read_pixels(
        sdl::rect{.x = x, .y = y, .width = 1, .height = 1}
    );
    if (!read) return {.r = 1, .g = 2, .b = 3, .a = 4};
    auto const color = (*read)->read_pixel({.x = 0, .y = 0});
    return color ? *color : sdl::rgba{.r = 1, .g = 2, .b = 3, .a = 4};
}

auto is_black(sdl::renderer renderer, int x, int y) -> bool
{
    return same(pixel(renderer, x, y), black);
}

void fill_with(sdl::renderer renderer, sdl::rgba color)
{
    (void)renderer.set_draw_color(color);
    (void)renderer.clear();
}

auto make_target(sdl::extent size) -> sdl::unique_surface
{
    auto created = sdl::surface::create(size, sdl::pixel_format::rgba32);
    return created ? std::move(*created) : sdl::unique_surface();
}

// A texture filled with one color.
auto make_texture(
    sdl::renderer renderer, sdl::extent size, sdl::rgba color,
    sdl::texture_access access = sdl::texture_access::static_
) -> sdl::unique_texture
{
    auto made =
        sdl::texture::create(renderer, sdl::pixel_format::rgba32, access, size);
    if (!made) return {};
    std::vector<std::byte> bytes(
        static_cast<std::size_t>(size.width * size.height * 4)
    );
    for (std::size_t i = 0; i != bytes.size(); i += 4) {
        bytes[i] = static_cast<std::byte>(color.r);
        bytes[i + 1] = static_cast<std::byte>(color.g);
        bytes[i + 2] = static_cast<std::byte>(color.b);
        bytes[i + 3] = static_cast<std::byte>(color.a);
    }
    if (access != sdl::texture_access::target)
        (void)(*made)->update(std::nullopt, bytes, size.width * 4);
    return std::move(*made);
}

} // namespace

int main()
{
    eggs::test_support::use_dummy_drivers();

    auto const subsystems = sdl::init(sdl::init_flags::video);
    EGGS_SDL_CHECK(subsystems.has_value());

    // -- Drivers ---------------------------------------------------------------

    EGGS_SDL_CHECK(sdl::render_driver_count() > 0);
    EGGS_SDL_CHECK(!sdl::render_driver(0).empty());
    EGGS_SDL_CHECK(sdl::render_driver(-1).empty());
    EGGS_SDL_CHECK(sdl::render_driver(sdl::render_driver_count()).empty());
    bool has_software = false;
    for (int i = 0; i != sdl::render_driver_count(); ++i)
        has_software = has_software || sdl::render_driver(i) == "software";
    EGGS_SDL_CHECK(has_software);

    // -- A software renderer on a surface --------------------------------------

    auto const canvas = make_target({.width = 64, .height = 48});
    EGGS_SDL_CHECK(canvas);
    auto const owned = sdl::renderer::create_software(canvas.get());
    EGGS_SDL_CHECK(owned.has_value());
    if (!owned) return eggs::test_support::report();
    sdl::renderer const r = owned->get();
    EGGS_SDL_CHECK(r);
    EGGS_SDL_CHECK(!sdl::renderer{});
    EGGS_SDL_CHECK(r.handle() != nullptr);

    EGGS_SDL_CHECK(r.name().has_value() && *r.name() == "software");
    // There is no window to get, and no vertical sync to ask for.
    EGGS_SDL_CHECK(!r.get_window().has_value());
    EGGS_SDL_CHECK(!r.get_window().error().message().empty());
    EGGS_SDL_CHECK(!r.set_vsync(sdl::vsync_mode::every_refresh).has_value());
    auto const output = r.output_size();
    EGGS_SDL_CHECK(output && output->width == 64 && output->height == 48);

    // -- Frame and draw color --------------------------------------------------

    EGGS_SDL_CHECK(r.set_draw_color(red).has_value());
    EGGS_SDL_CHECK(same(r.draw_color().value_or(black), red));
    EGGS_SDL_CHECK(
        r.set_draw_color(sdl::fcolor{.r = 0.0F, .g = 1.0F, .b = 0.0F})
            .has_value()
    );
    EGGS_SDL_CHECK(same(r.draw_color().value_or(black), green));
    EGGS_SDL_CHECK(
        r.set_draw_color(sdl::rgb{.r = 255, .g = 255, .b = 255}).has_value()
    );
    EGGS_SDL_CHECK(same(r.draw_color().value_or(black), white));

    fill_with(r, red);
    EGGS_SDL_CHECK(same(pixel(r, 0, 0), red));
    EGGS_SDL_CHECK(same(pixel(r, 63, 47), red));
    EGGS_SDL_CHECK(r.present().has_value());
    EGGS_SDL_CHECK(r.flush().has_value());

    // -- Blend mode ------------------------------------------------------------

    EGGS_SDL_CHECK(r.set_draw_blend_mode(sdl::blend_mode::blend).has_value());
    EGGS_SDL_CHECK(r.get_draw_blend_mode() == sdl::blend_mode::blend);
    EGGS_SDL_CHECK(r.set_draw_blend_mode(sdl::blend_mode::none).has_value());
    EGGS_SDL_CHECK(r.get_draw_blend_mode() == sdl::blend_mode::none);

    // -- Primitives ------------------------------------------------------------

    fill_with(r, black);
    (void)r.set_draw_color(green);
    EGGS_SDL_CHECK(r.draw_point({.x = 5.0F, .y = 5.0F}).has_value());
    EGGS_SDL_CHECK(same(pixel(r, 5, 5), green));
    EGGS_SDL_CHECK(is_black(r, 6, 5));

    std::array<sdl::fpoint, 2> const points = {
        sdl::fpoint{.x = 10.0F, .y = 10.0F}, sdl::fpoint{.x = 12.0F, .y = 10.0F}
    };
    EGGS_SDL_CHECK(r.draw_points(points).has_value());
    EGGS_SDL_CHECK(same(pixel(r, 10, 10), green));
    EGGS_SDL_CHECK(same(pixel(r, 12, 10), green));
    EGGS_SDL_CHECK(is_black(r, 11, 10));

    EGGS_SDL_CHECK(
        r.draw_line({.x = 0.0F, .y = 20.0F}, {.x = 10.0F, .y = 20.0F})
            .has_value()
    );
    EGGS_SDL_CHECK(same(pixel(r, 5, 20), green));

    // A connected polyline: the corner and both ends.
    std::array<sdl::fpoint, 3> const polyline = {
        sdl::fpoint{.x = 20.0F, .y = 30.0F},
        sdl::fpoint{.x = 30.0F, .y = 30.0F}, sdl::fpoint{.x = 30.0F, .y = 40.0F}
    };
    EGGS_SDL_CHECK(r.draw_lines(polyline).has_value());
    EGGS_SDL_CHECK(same(pixel(r, 20, 30), green));
    EGGS_SDL_CHECK(same(pixel(r, 30, 30), green));
    EGGS_SDL_CHECK(same(pixel(r, 30, 40), green));
    EGGS_SDL_CHECK(is_black(r, 20, 40));

    fill_with(r, black);
    (void)r.set_draw_color(red);
    EGGS_SDL_CHECK(
        r.fill_rect({.x = 4.0F, .y = 4.0F, .width = 4.0F, .height = 4.0F})
            .has_value()
    );
    EGGS_SDL_CHECK(same(pixel(r, 4, 4), red));
    EGGS_SDL_CHECK(same(pixel(r, 7, 7), red));
    EGGS_SDL_CHECK(is_black(r, 8, 8));

    EGGS_SDL_CHECK(
        r.draw_rect({.x = 20.0F, .y = 4.0F, .width = 6.0F, .height = 6.0F})
            .has_value()
    );
    EGGS_SDL_CHECK(same(pixel(r, 20, 4), red));
    EGGS_SDL_CHECK(same(pixel(r, 25, 9), red));
    EGGS_SDL_CHECK(is_black(r, 22, 6));

    std::array<sdl::frect, 2> const areas = {
        sdl::frect{.x = 30.0F, .y = 4.0F, .width = 2.0F, .height = 2.0F},
        sdl::frect{.x = 40.0F, .y = 4.0F, .width = 2.0F, .height = 2.0F}
    };
    EGGS_SDL_CHECK(r.fill_rects(areas).has_value());
    EGGS_SDL_CHECK(same(pixel(r, 31, 5), red));
    EGGS_SDL_CHECK(same(pixel(r, 41, 5), red));
    EGGS_SDL_CHECK(r.draw_rects(areas).has_value());

    // Nothing to draw is not an error.
    EGGS_SDL_CHECK(r.draw_points({}).has_value());
    EGGS_SDL_CHECK(r.draw_lines({}).has_value());
    EGGS_SDL_CHECK(r.draw_rects({}).has_value());
    EGGS_SDL_CHECK(r.fill_rects({}).has_value());

    // -- Viewport --------------------------------------------------------------

    fill_with(r, black);
    (void)r.set_draw_color(red);
    EGGS_SDL_CHECK(
        r.set_viewport(sdl::rect{.x = 8, .y = 8, .width = 16, .height = 16})
            .has_value()
    );
    auto const viewport = r.viewport();
    EGGS_SDL_CHECK(
        viewport && viewport->x == 8 && viewport->y == 8 &&
        viewport->width == 16 && viewport->height == 16
    );
    EGGS_SDL_CHECK(
        r.fill_rect({.x = 0.0F, .y = 0.0F, .width = 4.0F, .height = 4.0F})
            .has_value()
    );
    EGGS_SDL_CHECK(r.set_viewport(std::nullopt).has_value());
    EGGS_SDL_CHECK(same(pixel(r, 8, 8), red));
    EGGS_SDL_CHECK(is_black(r, 0, 0));
    auto const whole = r.viewport();
    EGGS_SDL_CHECK(
        whole && whole->x == 0 && whole->y == 0 && whole->width == 64 &&
        whole->height == 48
    );

    // -- Clip rect -------------------------------------------------------------

    fill_with(r, black);
    (void)r.set_draw_color(red);
    EGGS_SDL_CHECK(!r.clip_enabled());
    EGGS_SDL_CHECK(
        r.set_clip_rect(sdl::rect{.x = 10, .y = 10, .width = 10, .height = 10})
            .has_value()
    );
    EGGS_SDL_CHECK(r.clip_enabled());
    auto const clip = r.clip_rect();
    EGGS_SDL_CHECK(
        clip && clip->x == 10 && clip->y == 10 && clip->width == 10 &&
        clip->height == 10
    );
    EGGS_SDL_CHECK(
        r.fill_rect({.x = 0.0F, .y = 0.0F, .width = 64.0F, .height = 48.0F})
            .has_value()
    );
    EGGS_SDL_CHECK(same(pixel(r, 12, 12), red));
    EGGS_SDL_CHECK(is_black(r, 5, 5));
    EGGS_SDL_CHECK(r.set_clip_rect(std::nullopt).has_value());
    EGGS_SDL_CHECK(!r.clip_enabled());

    // -- Scale -----------------------------------------------------------------

    fill_with(r, black);
    (void)r.set_draw_color(red);
    EGGS_SDL_CHECK(r.set_scale({.x = 2.0F, .y = 2.0F}).has_value());
    auto const scale = r.scale();
    EGGS_SDL_CHECK(scale && scale->x == 2.0F && scale->y == 2.0F);
    EGGS_SDL_CHECK(
        r.fill_rect({.x = 0.0F, .y = 0.0F, .width = 4.0F, .height = 4.0F})
            .has_value()
    );
    EGGS_SDL_CHECK(r.set_scale({.x = 1.0F, .y = 1.0F}).has_value());
    EGGS_SDL_CHECK(same(pixel(r, 7, 7), red));
    EGGS_SDL_CHECK(is_black(r, 8, 8));

    // -- Logical presentation and coordinates ----------------------------------

    auto const initially = r.logical_presentation();
    EGGS_SDL_CHECK(
        initially && initially->mode == sdl::logical_presentation_mode::disabled
    );
    EGGS_SDL_CHECK(r.set_logical_presentation(
                        {.width = 32, .height = 24},
                        sdl::logical_presentation_mode::stretch
    )
                       .has_value());
    auto const info = r.logical_presentation();
    EGGS_SDL_CHECK(
        info && info->size.width == 32 && info->size.height == 24 &&
        info->mode == sdl::logical_presentation_mode::stretch
    );
    auto const to_window = r.coordinates_to_window({.x = 16.0F, .y = 12.0F});
    EGGS_SDL_CHECK(to_window && near(*to_window, {.x = 32.0F, .y = 24.0F}));
    auto const from_window =
        r.coordinates_from_window({.x = 32.0F, .y = 24.0F});
    EGGS_SDL_CHECK(from_window && near(*from_window, {.x = 16.0F, .y = 12.0F}));
    fill_with(r, black);
    (void)r.set_draw_color(red);
    EGGS_SDL_CHECK(
        r.fill_rect({.x = 0.0F, .y = 0.0F, .width = 2.0F, .height = 2.0F})
            .has_value()
    );
    EGGS_SDL_CHECK(same(pixel(r, 3, 3), red));
    EGGS_SDL_CHECK(is_black(r, 4, 4));
    EGGS_SDL_CHECK(r.set_logical_presentation(
                        {.width = 0, .height = 0},
                        sdl::logical_presentation_mode::disabled
    )
                       .has_value());
    auto const disabled = r.logical_presentation();
    EGGS_SDL_CHECK(
        disabled && disabled->mode == sdl::logical_presentation_mode::disabled
    );

    // -- Textures --------------------------------------------------------------

    auto const small = make_texture(r, {.width = 8, .height = 8}, green);
    EGGS_SDL_CHECK(small);
    if (small) {
        sdl::texture const texture = small.get();
        EGGS_SDL_CHECK(texture.size().width == 8 && texture.size().height == 8);
        EGGS_SDL_CHECK(texture.format() == sdl::pixel_format::rgba32);

        fill_with(r, black);
        EGGS_SDL_CHECK(
            r.draw_texture(
                 texture, {},
                 sdl::frect{.x = 4.0F, .y = 4.0F, .width = 8.0F, .height = 8.0F}
            )
                .has_value()
        );
        EGGS_SDL_CHECK(same(pixel(r, 4, 4), green));
        EGGS_SDL_CHECK(same(pixel(r, 11, 11), green));
        EGGS_SDL_CHECK(is_black(r, 12, 12));

        // The whole texture on the whole target.
        fill_with(r, black);
        EGGS_SDL_CHECK(r.draw_texture(texture).has_value());
        EGGS_SDL_CHECK(same(pixel(r, 60, 40), green));

        // A part of the texture, stretched.
        fill_with(r, black);
        EGGS_SDL_CHECK(
            r.draw_texture(
                 texture,
                 sdl::frect{
                     .x = 0.0F, .y = 0.0F, .width = 2.0F, .height = 2.0F
                 },
                 sdl::frect{
                     .x = 0.0F, .y = 0.0F, .width = 16.0F, .height = 16.0F
                 }
            )
                .has_value()
        );
        EGGS_SDL_CHECK(same(pixel(r, 15, 15), green));

        sdl::frect const destination = {
            .x = 8.0F, .y = 8.0F, .width = 16.0F, .height = 16.0F
        };
        fill_with(r, black);
        EGGS_SDL_CHECK(
            r.draw_texture_rotated(texture, {}, destination, 45.0).has_value()
        );
        EGGS_SDL_CHECK(same(pixel(r, 16, 16), green));
        EGGS_SDL_CHECK(r.draw_texture_rotated(
                            texture, {}, destination, 90.0,
                            sdl::fpoint{.x = 0.0F, .y = 0.0F},
                            sdl::flip_mode::horizontal
        )
                           .has_value());

        fill_with(r, black);
        EGGS_SDL_CHECK(
            r.draw_texture_tiled(texture, {}, 1.0F, destination).has_value()
        );
        EGGS_SDL_CHECK(same(pixel(r, 10, 10), green));
        EGGS_SDL_CHECK(same(pixel(r, 22, 22), green));
        EGGS_SDL_CHECK(is_black(r, 24, 24));

        fill_with(r, black);
        EGGS_SDL_CHECK(r.draw_texture_9grid(
                            texture, {}, 2.0F, 2.0F, 2.0F, 2.0F, 1.0F,
                            destination
        )
                           .has_value());
        EGGS_SDL_CHECK(same(pixel(r, 8, 8), green));
        EGGS_SDL_CHECK(same(pixel(r, 16, 16), green));
        EGGS_SDL_CHECK(same(pixel(r, 23, 23), green));

        // -- Modulation and modes
        EGGS_SDL_CHECK(
            texture.set_color_mod({.r = 255, .g = 0, .b = 0}).has_value()
        );
        auto const mod = texture.color_mod();
        EGGS_SDL_CHECK(mod && mod->r == 255 && mod->g == 0 && mod->b == 0);
        fill_with(r, black);
        EGGS_SDL_CHECK(r.draw_texture(texture).has_value());
        EGGS_SDL_CHECK(is_black(r, 10, 10));
        EGGS_SDL_CHECK(
            texture.set_color_mod({.r = 255, .g = 255, .b = 255}).has_value()
        );

        EGGS_SDL_CHECK(texture.set_alpha_mod(128).has_value());
        EGGS_SDL_CHECK(texture.alpha_mod() == std::uint8_t{128});
        EGGS_SDL_CHECK(texture.set_alpha_mod(255).has_value());
        EGGS_SDL_CHECK(
            texture.set_blend_mode(sdl::blend_mode::add).has_value()
        );
        EGGS_SDL_CHECK(texture.get_blend_mode() == sdl::blend_mode::add);
        EGGS_SDL_CHECK(
            texture.set_scale_mode(sdl::scale_mode::linear).has_value()
        );
        EGGS_SDL_CHECK(texture.get_scale_mode() == sdl::scale_mode::linear);
        EGGS_SDL_CHECK(
            texture.set_scale_mode(sdl::scale_mode::nearest).has_value()
        );
        EGGS_SDL_CHECK(texture.get_scale_mode() == sdl::scale_mode::nearest);
    }

    // -- Updating a texture ----------------------------------------------------

    auto const updated = sdl::texture::create(
        r, sdl::pixel_format::rgba32, sdl::texture_access::static_,
        {.width = 4, .height = 4}
    );
    EGGS_SDL_CHECK(updated.has_value());
    if (updated) {
        sdl::texture const texture = updated->get();
        std::vector<std::byte> bytes(std::size_t{64}, std::byte{255});
        EGGS_SDL_CHECK(texture.update(std::nullopt, bytes, 16).has_value());
        fill_with(r, black);
        EGGS_SDL_CHECK(r.draw_texture(texture).has_value());
        EGGS_SDL_CHECK(same(pixel(r, 30, 20), white));

        // A part of it.
        std::vector<std::byte> part(std::size_t{16}, std::byte{0});
        EGGS_SDL_CHECK(
            texture
                .update(
                    sdl::rect{.x = 0, .y = 0, .width = 2, .height = 2}, part, 8
                )
                .has_value()
        );

        // A span that is too small fails, and does not read past its end.
        std::span<std::byte const> const too_small(bytes.data(), 8);
        auto const small_update = texture.update(std::nullopt, too_small, 16);
        EGGS_SDL_CHECK(!small_update.has_value());
        EGGS_SDL_CHECK(!small_update.error().message().empty());
    }

    // -- Locking a streaming texture -------------------------------------------

    auto const streaming = sdl::texture::create(
        r, sdl::pixel_format::rgba32, sdl::texture_access::streaming,
        {.width = 8, .height = 8}
    );
    EGGS_SDL_CHECK(streaming.has_value());
    if (streaming) {
        sdl::texture const texture = streaming->get();
        {
            auto locked = texture.lock();
            EGGS_SDL_CHECK(locked.has_value());
            if (locked) {
                EGGS_SDL_CHECK(locked->pitch() >= 8 * 4);
                EGGS_SDL_CHECK(
                    locked->pixels().size() >=
                    (static_cast<std::size_t>(locked->pitch()) * 7U) + 32U
                );
                for (std::byte& byte : locked->pixels()) byte = std::byte{255};

                // Moving the lock hands it over, and unlocks once.
                sdl::texture_lock const moved = std::move(*locked);
                EGGS_SDL_CHECK(locked->pixels().empty());
                EGGS_SDL_CHECK(locked->pitch() == 0);
                EGGS_SDL_CHECK(!moved.pixels().empty());
            }
        }
        fill_with(r, black);
        EGGS_SDL_CHECK(r.draw_texture(texture).has_value());
        EGGS_SDL_CHECK(same(pixel(r, 30, 20), white));

        // The span of a part is as long as the part.
        auto const part =
            texture.lock(sdl::rect{.x = 2, .y = 2, .width = 4, .height = 3});
        EGGS_SDL_CHECK(part.has_value());
        if (part) {
            EGGS_SDL_CHECK(
                part->pixels().size() ==
                (static_cast<std::size_t>(part->pitch()) * 2U) + 16U
            );
        }
    }

    // Only streaming textures lock.
    auto const frozen = make_texture(r, {.width = 4, .height = 4}, white);
    EGGS_SDL_CHECK(frozen);
    if (frozen) {
        auto const refused = frozen->lock();
        EGGS_SDL_CHECK(!refused.has_value());
    }

    // -- A texture from a surface ----------------------------------------------

    {
        auto source = sdl::surface::create(
            {.width = 4, .height = 4}, sdl::pixel_format::rgba32
        );
        EGGS_SDL_CHECK(source.has_value());
        if (source) {
            (void)(*source)->fill(green);
            auto const copied = sdl::texture::from_surface(r, source->get());
            EGGS_SDL_CHECK(copied.has_value());
            // The surface can be released: the pixels were copied.
            source->reset();
            if (copied) {
                EGGS_SDL_CHECK((*copied)->size().width == 4);
                fill_with(r, black);
                EGGS_SDL_CHECK(r.draw_texture(copied->get()).has_value());
                EGGS_SDL_CHECK(same(pixel(r, 30, 20), green));
            }
        }
    }

    // -- Render targets --------------------------------------------------------

    auto const offscreen = make_texture(
        r, {.width = 16, .height = 16}, black, sdl::texture_access::target
    );
    EGGS_SDL_CHECK(offscreen);
    if (offscreen) {
        sdl::texture const texture = offscreen.get();
        EGGS_SDL_CHECK(!r.target());
        EGGS_SDL_CHECK(r.set_target(texture).has_value());
        EGGS_SDL_CHECK(r.target() == texture);
        fill_with(r, red);
        EGGS_SDL_CHECK(r.reset_target().has_value());
        EGGS_SDL_CHECK(!r.target());

        fill_with(r, black);
        EGGS_SDL_CHECK(
            r.draw_texture(
                 texture, {},
                 sdl::frect{
                     .x = 0.0F, .y = 0.0F, .width = 16.0F, .height = 16.0F
                 }
            )
                .has_value()
        );
        EGGS_SDL_CHECK(same(pixel(r, 8, 8), red));
    }

    // Only a target texture can be drawn on.
    if (frozen) {
        EGGS_SDL_CHECK(!r.set_target(frozen.get()).has_value());
        EGGS_SDL_CHECK(!r.target());
    }

    // -- Geometry --------------------------------------------------------------

    {
        constexpr sdl::fcolor all_red = {.r = 1.0F, .g = 0.0F, .b = 0.0F};
        std::array<sdl::vertex, 3> const triangle = {
            sdl::vertex{
                .position = {.x = 0.0F, .y = 0.0F},
                .color = all_red,
                .tex_coord = {}
            },
            sdl::vertex{
                .position = {.x = 40.0F, .y = 0.0F},
                .color = all_red,
                .tex_coord = {}
            },
            sdl::vertex{
                .position = {.x = 0.0F, .y = 40.0F},
                .color = all_red,
                .tex_coord = {}
            }
        };
        fill_with(r, black);
        EGGS_SDL_CHECK(r.draw_geometry(sdl::texture{}, triangle).has_value());
        EGGS_SDL_CHECK(same(pixel(r, 5, 5), red));
        EGGS_SDL_CHECK(is_black(r, 40, 40));

        std::array<int, 3> const indices = {2, 1, 0};
        fill_with(r, black);
        EGGS_SDL_CHECK(
            r.draw_geometry(sdl::texture{}, triangle, indices).has_value()
        );
        EGGS_SDL_CHECK(same(pixel(r, 5, 5), red));

        std::array<int, 3> const outside = {0, 1, 7};
        EGGS_SDL_CHECK(
            !r.draw_geometry(sdl::texture{}, triangle, outside).has_value()
        );
        EGGS_SDL_CHECK(r.draw_geometry(sdl::texture{}, {}).has_value());
    }

    // -- Debug text ------------------------------------------------------------

    fill_with(r, black);
    (void)r.set_draw_color(white);
    EGGS_SDL_CHECK(r.draw_debug_text({.x = 0.0F, .y = 0.0F}, "A").has_value());
    {
        auto const text =
            r.read_pixels(sdl::rect{.x = 0, .y = 0, .width = 8, .height = 8});
        EGGS_SDL_CHECK(text.has_value());
        bool drawn = false;
        if (text) {
            for (int y = 0; y != 8; ++y)
                for (int x = 0; x != 8; ++x)
                    drawn = drawn || same(
                                         (*text)
                                             ->read_pixel({.x = x, .y = y})
                                             .value_or(black),
                                         white
                                     );
        }
        EGGS_SDL_CHECK(drawn);
    }

    // -- Reading pixels --------------------------------------------------------

    {
        fill_with(r, red);
        auto const all = r.read_pixels();
        EGGS_SDL_CHECK(all.has_value());
        if (all) {
            EGGS_SDL_CHECK((*all)->size().width == 64);
            EGGS_SDL_CHECK((*all)->size().height == 48);
            EGGS_SDL_CHECK(same(
                (*all)->read_pixel({.x = 10, .y = 10}).value_or(black), red
            ));
        }
        auto const part =
            r.read_pixels(sdl::rect{.x = 8, .y = 8, .width = 4, .height = 2});
        EGGS_SDL_CHECK(
            part && (*part)->size().width == 4 && (*part)->size().height == 2
        );
        EGGS_SDL_CHECK(
            !r.read_pixels(sdl::rect{.x = 0, .y = 0, .width = -1, .height = 4})
                 .has_value()
        );
    }

    // -- A texture of another renderer -----------------------------------------

    {
        auto const other_canvas = make_target({.width = 16, .height = 16});
        auto const other = sdl::renderer::create_software(other_canvas.get());
        EGGS_SDL_CHECK(other.has_value());
        if (other) {
            auto const foreign =
                make_texture(other->get(), {.width = 4, .height = 4}, white);
            EGGS_SDL_CHECK(foreign);
            if (foreign) {
                EGGS_SDL_CHECK(!r.draw_texture(foreign.get()).has_value());
                EGGS_SDL_CHECK(
                    other->get().draw_texture(foreign.get()).has_value()
                );
            }
        }
    }

    // -- Errors ----------------------------------------------------------------

    EGGS_SDL_CHECK(!sdl::texture::create(
                        r, sdl::pixel_format::rgba32,
                        sdl::texture_access::static_, {.width = -1, .height = 4}
    )
                        .has_value());

    // -- A renderer for a window -----------------------------------------------

    auto const window = sdl::window::create("Eggs.SDL renderer test", 200, 100);
    EGGS_SDL_CHECK(window.has_value());
    if (window) {
        EGGS_SDL_CHECK(
            !sdl::renderer::create(window->get(), "no such driver").has_value()
        );
        auto const created = sdl::renderer::create(window->get());
        EGGS_SDL_CHECK(created.has_value());
        if (created) {
            sdl::renderer const w = created->get();
            EGGS_SDL_CHECK(w.name().has_value());
            EGGS_SDL_CHECK(w.get_window() == window->get());
            auto const size = w.output_size();
            EGGS_SDL_CHECK(size && size->width == 200 && size->height == 100);

            // Vertical sync may be refused, what it says is what it does.
            if (w.set_vsync(sdl::vsync_mode::every_refresh)) {
                EGGS_SDL_CHECK(w.get_vsync() == sdl::vsync_mode::every_refresh);
            }
            if (w.set_vsync(sdl::vsync_mode::disabled)) {
                EGGS_SDL_CHECK(w.get_vsync() == sdl::vsync_mode::disabled);
            }

            // The events of the window are converted to the logical space.
            EGGS_SDL_CHECK(w.set_logical_presentation(
                                {.width = 100, .height = 50},
                                sdl::logical_presentation_mode::stretch
            )
                               .has_value());
            sdl::event motion;
            motion.sdl().type = SDL_EVENT_MOUSE_MOTION;
            motion.sdl().motion.windowID =
                SDL_GetWindowID(window->get().handle());
            motion.sdl().motion.x = 100.0F;
            motion.sdl().motion.y = 50.0F;
            EGGS_SDL_CHECK(w.convert_event_coordinates(motion).has_value());
            auto const converted = motion.get_if<sdl::mouse_motion_event>();
            EGGS_SDL_CHECK(converted);
            if (converted) {
                EGGS_SDL_CHECK(converted->x() == 50.0F);
                EGGS_SDL_CHECK(converted->y() == 25.0F);
            }

            // Other events are left as they are.
            sdl::event key;
            key.sdl().type = SDL_EVENT_KEY_DOWN;
            key.sdl().key.scancode = SDL_SCANCODE_A;
            EGGS_SDL_CHECK(w.convert_event_coordinates(key).has_value());
            EGGS_SDL_CHECK(key.holds_alternative<sdl::key_down_event>());

            // A frame, drawn in full and presented.
            fill_with(w, black);
            EGGS_SDL_CHECK(w.present().has_value());
        }
    }

    return eggs::test_support::report();
}
