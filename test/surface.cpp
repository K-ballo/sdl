// Eggs.SDL
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#include <eggs/sdl.hpp>

#include <array>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>

#include "support.hpp"

namespace sdl = eggs::sdl;

namespace {

bool same(sdl::rgba lhs, sdl::rgba rhs)
{
    return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b && lhs.a == rhs.a;
}

auto pixel(sdl::surface surface, int x, int y) -> sdl::rgba
{
    auto const result = surface.read_pixel({.x = x, .y = y});
    EGGS_SDL_CHECK(result.has_value());
    return result.value_or(sdl::rgba{});
}

// Every surface here is created with the memory order r, g, b, a.
auto make(int width, int height) -> sdl::unique_surface
{
    auto created = sdl::surface::create(
        {.width = width, .height = height}, sdl::pixel_format::rgba32
    );
    EGGS_SDL_CHECK(created.has_value());
    if (!created) std::exit(eggs::test_support::report());
    return *std::move(created);
}

constexpr sdl::rgba red = {.r = 255, .g = 0, .b = 0};
constexpr sdl::rgba green = {.r = 0, .g = 255, .b = 0};
constexpr sdl::rgba blue = {.r = 0, .g = 0, .b = 255};

} // namespace

int main()
{
    eggs::test_support::use_dummy_drivers();

    // -- Create and query ------------------------------------------------------

    sdl::unique_surface const owner = make(8, 6);
    sdl::surface const surface = owner.get();
    EGGS_SDL_CHECK(surface);
    EGGS_SDL_CHECK(surface.size().width == 8 && surface.size().height == 6);
    EGGS_SDL_CHECK(surface.format() == sdl::pixel_format::rgba32);
    EGGS_SDL_CHECK(surface.pitch() >= 8 * 4);

    auto const empty = sdl::surface::create(
        {.width = -1, .height = 4}, sdl::pixel_format::rgba32
    );
    EGGS_SDL_CHECK(!empty.has_value());
    EGGS_SDL_CHECK(!empty.error().message().empty());

    // -- Fill, clear and single pixels ----------------------------------------

    EGGS_SDL_CHECK(surface.fill(red).has_value());
    EGGS_SDL_CHECK(same(pixel(surface, 0, 0), red));
    EGGS_SDL_CHECK(same(pixel(surface, 7, 5), red));

    // An rgb converts to rgba, opaque.
    EGGS_SDL_CHECK(surface.fill(sdl::rgb{0, 255, 0}).has_value());
    EGGS_SDL_CHECK(same(pixel(surface, 3, 3), green));

    EGGS_SDL_CHECK(surface.fill({.x = 1, .y = 1, .width = 2, .height = 2}, blue)
                       .has_value());
    EGGS_SDL_CHECK(same(pixel(surface, 1, 1), blue));
    EGGS_SDL_CHECK(same(pixel(surface, 2, 2), blue));
    EGGS_SDL_CHECK(same(pixel(surface, 3, 3), green));

    std::array<sdl::rect, 2> const areas = {
        sdl::rect{.x = 0, .y = 0, .width = 1, .height = 1},
        sdl::rect{.x = 7, .y = 5, .width = 1, .height = 1}
    };
    EGGS_SDL_CHECK(surface.fill(areas, red).has_value());
    EGGS_SDL_CHECK(same(pixel(surface, 0, 0), red));
    EGGS_SDL_CHECK(same(pixel(surface, 7, 5), red));
    EGGS_SDL_CHECK(same(pixel(surface, 4, 4), green));

    EGGS_SDL_CHECK(surface.clear({0.0F, 0.0F, 1.0F, 1.0F}).has_value());
    EGGS_SDL_CHECK(same(pixel(surface, 4, 4), blue));

    EGGS_SDL_CHECK(surface.write_pixel({5, 4}, {10, 20, 30, 40}).has_value());
    EGGS_SDL_CHECK(same(pixel(surface, 5, 4), {10, 20, 30, 40}));
    EGGS_SDL_CHECK(!surface.read_pixel({100, 100}).has_value());
    EGGS_SDL_CHECK(!surface.write_pixel({-1, 0}, red).has_value());

    // -- Lock ------------------------------------------------------------------

    EGGS_SDL_CHECK(surface.fill(blue).has_value());
    {
        auto locked = surface.lock();
        EGGS_SDL_CHECK(locked.has_value());
        if (locked) {
            EGGS_SDL_CHECK(locked->pitch() == surface.pitch());
            EGGS_SDL_CHECK(
                locked->pixels().size() ==
                static_cast<std::size_t>(surface.pitch()) * 6
            );
            // r, g, b, a in memory order.
            EGGS_SDL_CHECK(locked->pixels()[0] == std::byte{0});
            EGGS_SDL_CHECK(locked->pixels()[2] == std::byte{255});
            locked->pixels()[0] = std::byte{255};

            sdl::surface_lock const moved = *std::move(locked);
            EGGS_SDL_CHECK(!moved.pixels().empty());
            // NOLINTNEXTLINE(bugprone-use-after-move): a moved-from lock is empty
            EGGS_SDL_CHECK(locked->pixels().empty());
        }
    }
    EGGS_SDL_CHECK(same(pixel(surface, 0, 0), {255, 0, 255}));

    // -- Wrapped memory --------------------------------------------------------

    {
        std::array<std::byte, 16> memory{};
        auto wrapped = sdl::surface::from_pixels(
            {.width = 2, .height = 2}, sdl::pixel_format::rgba32, memory, 8
        );
        EGGS_SDL_CHECK(wrapped.has_value());
        if (wrapped) {
            EGGS_SDL_CHECK((*wrapped)->fill(green).has_value());
            // The surface drew into the caller's memory.
            EGGS_SDL_CHECK(memory[1] == std::byte{255});
            EGGS_SDL_CHECK(memory[0] == std::byte{0});
        }

        auto const too_small = sdl::surface::from_pixels(
            {.width = 2, .height = 3}, sdl::pixel_format::rgba32, memory, 8
        );
        EGGS_SDL_CHECK(!too_small.has_value());
        EGGS_SDL_CHECK(!too_small.error().message().empty());
        auto const bad_pitch = sdl::surface::from_pixels(
            {.width = 2, .height = 2}, sdl::pixel_format::rgba32, memory, 4
        );
        EGGS_SDL_CHECK(!bad_pitch.has_value());
    }

    // -- Blit ------------------------------------------------------------------

    {
        sdl::unique_surface const source_owner = make(2, 2);
        sdl::surface const source = source_owner.get();
        EGGS_SDL_CHECK(source.fill(red).has_value());
        EGGS_SDL_CHECK(source.write_pixel({1, 1}, green).has_value());

        sdl::unique_surface const destination_owner = make(6, 6);
        sdl::surface const destination = destination_owner.get();
        EGGS_SDL_CHECK(destination.fill(blue).has_value());

        EGGS_SDL_CHECK(source.blit(destination, {2, 3}).has_value());
        EGGS_SDL_CHECK(same(pixel(destination, 2, 3), red));
        EGGS_SDL_CHECK(same(pixel(destination, 3, 4), green));
        EGGS_SDL_CHECK(same(pixel(destination, 1, 3), blue));
        EGGS_SDL_CHECK(same(pixel(destination, 4, 4), blue));

        // Only the position of the destination is used, a part of the source
        // is what changes the size.
        EGGS_SDL_CHECK(destination.fill(blue).has_value());
        EGGS_SDL_CHECK(
            source
                .blit(
                    destination, {0, 0},
                    sdl::rect{.x = 1, .y = 1, .width = 1, .height = 1}
                )
                .has_value()
        );
        EGGS_SDL_CHECK(same(pixel(destination, 0, 0), green));
        EGGS_SDL_CHECK(same(pixel(destination, 1, 0), blue));
        EGGS_SDL_CHECK(same(pixel(destination, 0, 1), blue));

        // Blitting off the edge draws what is inside.
        EGGS_SDL_CHECK(destination.fill(blue).has_value());
        EGGS_SDL_CHECK(source.blit(destination, {5, 5}).has_value());
        EGGS_SDL_CHECK(same(pixel(destination, 5, 5), red));

        EGGS_SDL_CHECK(destination.fill(blue).has_value());
        EGGS_SDL_CHECK(source
                           .blit_scaled(
                               destination,
                               {.x = 0, .y = 0, .width = 4, .height = 4},
                               sdl::scale_mode::nearest
                           )
                           .has_value());
        EGGS_SDL_CHECK(same(pixel(destination, 0, 0), red));
        EGGS_SDL_CHECK(same(pixel(destination, 3, 3), green));
        EGGS_SDL_CHECK(same(pixel(destination, 4, 4), blue));

        // -- Different formats, the blit converts

        auto const converted = source.convert(sdl::pixel_format::rgb565);
        EGGS_SDL_CHECK(converted.has_value());
        if (converted) {
            EGGS_SDL_CHECK((*converted)->format() == sdl::pixel_format::rgb565);
            EGGS_SDL_CHECK(destination.fill(blue).has_value());
            EGGS_SDL_CHECK((*converted)->blit(destination, {0, 0}).has_value());
            EGGS_SDL_CHECK(same(pixel(destination, 0, 0), red));
            // The source was not modified.
            EGGS_SDL_CHECK(source.format() == sdl::pixel_format::rgba32);
        }

        // -- Color key, modulation and blend mode

        EGGS_SDL_CHECK(!source.color_key().has_value());
        EGGS_SDL_CHECK(source.set_color_key(red).has_value());
        EGGS_SDL_CHECK(source.color_key().has_value());
        EGGS_SDL_CHECK(
            source.color_key() && same(*source.color_key(), {255, 0, 0, 255})
        );
        EGGS_SDL_CHECK(destination.fill(blue).has_value());
        EGGS_SDL_CHECK(source.blit(destination, {0, 0}).has_value());
        EGGS_SDL_CHECK(same(pixel(destination, 0, 0), blue));
        EGGS_SDL_CHECK(same(pixel(destination, 1, 1), green));
        EGGS_SDL_CHECK(source.set_color_key(std::nullopt).has_value());
        EGGS_SDL_CHECK(!source.color_key().has_value());

        EGGS_SDL_CHECK(source.set_color_mod({128, 255, 255}).has_value());
        auto const color_mod = source.color_mod();
        EGGS_SDL_CHECK(color_mod && color_mod->r == 128 && color_mod->g == 255);
        EGGS_SDL_CHECK(source.set_alpha_mod(100).has_value());
        auto const alpha_mod = source.alpha_mod();
        EGGS_SDL_CHECK(alpha_mod && *alpha_mod == 100);

        EGGS_SDL_CHECK(source.set_blend_mode(sdl::blend_mode::add).has_value());
        auto const mode = source.get_blend_mode();
        EGGS_SDL_CHECK(mode && *mode == sdl::blend_mode::add);

        // -- Clip rect

        auto const full = destination.clip_rect();
        EGGS_SDL_CHECK(full && full->width == 6 && full->height == 6);
        EGGS_SDL_CHECK(destination.set_clip_rect(
            sdl::rect{.x = 1, .y = 1, .width = 2, .height = 2}
        ));
        auto const clipped = destination.clip_rect();
        EGGS_SDL_CHECK(clipped && clipped->x == 1 && clipped->width == 2);
        EGGS_SDL_CHECK(destination.fill(green).has_value());
        EGGS_SDL_CHECK(same(pixel(destination, 1, 1), green));
        EGGS_SDL_CHECK(same(pixel(destination, 0, 0), blue));
        // Outside the surface is not an error, only nothing is drawn.
        EGGS_SDL_CHECK(!destination.set_clip_rect(
            sdl::rect{.x = 100, .y = 100, .width = 2, .height = 2}
        ));
        EGGS_SDL_CHECK(destination.set_clip_rect(std::nullopt));
        auto const cleared = destination.clip_rect();
        EGGS_SDL_CHECK(cleared && cleared->width == 6 && cleared->height == 6);
    }

    // -- New surfaces ----------------------------------------------------------

    {
        EGGS_SDL_CHECK(surface.fill(red).has_value());
        auto const copy = surface.duplicate();
        EGGS_SDL_CHECK(copy.has_value());
        if (copy) {
            EGGS_SDL_CHECK((*copy)->handle() != surface.handle());
            EGGS_SDL_CHECK(same(pixel(copy->get(), 2, 2), red));
            EGGS_SDL_CHECK((*copy)->fill(green).has_value());
            EGGS_SDL_CHECK(same(pixel(surface, 2, 2), red));
        }

        auto const scaled =
            surface.scale({.width = 16, .height = 12}, sdl::scale_mode::linear);
        EGGS_SDL_CHECK(scaled.has_value());
        if (scaled) {
            EGGS_SDL_CHECK((*scaled)->size().width == 16);
            EGGS_SDL_CHECK((*scaled)->size().height == 12);
            EGGS_SDL_CHECK(surface.size().width == 8);
        }
        EGGS_SDL_CHECK(
            !surface.scale({.width = -1, .height = 4}, sdl::scale_mode::nearest)
                 .has_value()
        );
    }

    // -- In place --------------------------------------------------------------

    {
        sdl::unique_surface const flipped_owner = make(3, 2);
        sdl::surface const flipped = flipped_owner.get();
        EGGS_SDL_CHECK(flipped.fill(blue).has_value());
        EGGS_SDL_CHECK(flipped.write_pixel({0, 0}, red).has_value());
        EGGS_SDL_CHECK(flipped.flip(sdl::flip_mode::horizontal).has_value());
        EGGS_SDL_CHECK(same(pixel(flipped, 2, 0), red));
        EGGS_SDL_CHECK(same(pixel(flipped, 0, 0), blue));
        EGGS_SDL_CHECK(flipped.flip(sdl::flip_mode::vertical).has_value());
        EGGS_SDL_CHECK(same(pixel(flipped, 2, 1), red));

        EGGS_SDL_CHECK(flipped.fill({200, 100, 50, 128}).has_value());
        EGGS_SDL_CHECK(flipped.premultiply_alpha().has_value());
        sdl::rgba const premultiplied = pixel(flipped, 0, 0);
        EGGS_SDL_CHECK(premultiplied.a == 128);
        EGGS_SDL_CHECK(premultiplied.r < 120 && premultiplied.r > 90);
    }

    // -- Files -----------------------------------------------------------------

    {
        auto const path =
            (std::filesystem::temp_directory_path() / "eggs_sdl_surface.bmp")
                .string();
        EGGS_SDL_CHECK(surface.fill(green).has_value());
        EGGS_SDL_CHECK(surface.write_pixel({1, 2}, red).has_value());
        EGGS_SDL_CHECK(surface.save_bmp(path.c_str()).has_value());

        auto const loaded = sdl::surface::load_bmp(path.c_str());
        EGGS_SDL_CHECK(loaded.has_value());
        if (loaded) {
            EGGS_SDL_CHECK((*loaded)->size().width == 8);
            EGGS_SDL_CHECK((*loaded)->size().height == 6);
            EGGS_SDL_CHECK(same(pixel(loaded->get(), 1, 2), red));
            EGGS_SDL_CHECK(same(pixel(loaded->get(), 0, 0), green));
        }
        (void)std::remove(path.c_str());

        auto const missing = sdl::surface::load_bmp("no/such/file.bmp");
        EGGS_SDL_CHECK(!missing.has_value());
        EGGS_SDL_CHECK(!missing.error().message().empty());
    }

    // -- A window surface, borrowed ---------------------------------------------

    {
        auto const subsystems = sdl::init(sdl::init_flags::video);
        EGGS_SDL_CHECK(subsystems.has_value());
        if (!subsystems) return eggs::test_support::report();

        auto const created =
            sdl::window::create("surface", 64, 48, sdl::window_flags::hidden);
        EGGS_SDL_CHECK(created.has_value());
        if (!created) return eggs::test_support::report();

        auto const acquired = (*created)->get_surface();
        EGGS_SDL_CHECK(acquired.has_value());
        if (!acquired) return eggs::test_support::report();
        sdl::surface const window_surface = *acquired;

        EGGS_SDL_CHECK(window_surface.size().width == 64);
        EGGS_SDL_CHECK(sdl::bits_per_pixel(window_surface.format()) > 0);
        EGGS_SDL_CHECK(window_surface.fill(sdl::rgb{1, 2, 3}).has_value());
        EGGS_SDL_CHECK(window_surface.clear({0.0F, 0.0F, 0.0F}).has_value());

        // Convert once to the format of the window, then blit and scale onto it.
        auto const sprite = surface.convert(window_surface.format());
        EGGS_SDL_CHECK(sprite.has_value());
        if (sprite) {
            EGGS_SDL_CHECK((*sprite)->blit(window_surface, {4, 4}).has_value());
            EGGS_SDL_CHECK(
                (*sprite)
                    ->blit_scaled(
                        window_surface,
                        {.x = 20, .y = 20, .width = 16, .height = 16},
                        sdl::scale_mode::nearest
                    )
                    .has_value()
            );
        }
        {
            auto const locked = window_surface.lock();
            EGGS_SDL_CHECK(locked.has_value());
            EGGS_SDL_CHECK(locked && !locked->pixels().empty());
        }
        EGGS_SDL_CHECK((*created)->update_surface().has_value());
    }

    return eggs::test_support::report();
}
