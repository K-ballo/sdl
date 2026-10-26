// Eggs.SDL
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#include <eggs/sdl.hpp>

#include <array>
#include <cstdint>
#include <utility>

#include "support.hpp"

#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_version.h>

namespace sdl = eggs::sdl;

namespace {

auto pixel(sdl::surface surface, int x, int y) -> sdl::rgb
{
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
    std::uint8_t a = 0;
    SDL_ReadSurfacePixel(surface.handle(), x, y, &r, &g, &b, &a);
    return {.r = r, .g = g, .b = b};
}

bool same(sdl::rgb lhs, sdl::rgb rhs)
{
    return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b;
}

} // namespace

int main()
{
    eggs::test_support::use_dummy_drivers();

    auto const subsystems = sdl::init(sdl::init_flags::video);
    EGGS_SDL_CHECK(subsystems.has_value());
    if (!subsystems) return eggs::test_support::report();

    auto created = sdl::window::create(
        "title", 320, 200,
        sdl::window_flags::resizable | sdl::window_flags::hidden
    );
    EGGS_SDL_CHECK(created.has_value());
    if (!created) return eggs::test_support::report();

    sdl::unique_window owner = *std::move(created);
    EGGS_SDL_CHECK(owner);
    sdl::window const window = owner.get();
    EGGS_SDL_CHECK(window);
    EGGS_SDL_CHECK(window.handle() != nullptr);

    // -- Identity --------------------------------------------------------------

    auto const id = owner->id();
    EGGS_SDL_CHECK(id.has_value());
    EGGS_SDL_CHECK(id && *id != sdl::window_id{});
    auto const found = sdl::window::from_id(*id);
    EGGS_SDL_CHECK(found.has_value());
    EGGS_SDL_CHECK(found && *found == window);

    auto const missing = sdl::window::from_id(sdl::window_id{0x7FFFFFF});
    EGGS_SDL_CHECK(!missing.has_value());
    EGGS_SDL_CHECK(!missing.error().message().empty());

    // -- Title and size --------------------------------------------------------

    EGGS_SDL_CHECK(window.title() == "title");
    EGGS_SDL_CHECK(window.set_title("renamed").has_value());
    EGGS_SDL_CHECK(window.title() == "renamed");

    auto size = window.size();
    EGGS_SDL_CHECK(size.has_value());
    EGGS_SDL_CHECK(size && size->width == 320 && size->height == 200);
    EGGS_SDL_CHECK(window.set_size({400, 300}).has_value());
    size = window.size();
    EGGS_SDL_CHECK(size && size->width == 400 && size->height == 300);
    auto const pixel_size = window.pixel_size();
    EGGS_SDL_CHECK(pixel_size.has_value());
    EGGS_SDL_CHECK(
        pixel_size && pixel_size->width > 0 && pixel_size->height > 0
    );

    // -- Visibility ------------------------------------------------------------

    EGGS_SDL_CHECK(window.show().has_value());
    EGGS_SDL_CHECK(window.raise().has_value());
    EGGS_SDL_CHECK(window.hide().has_value());

    // -- Surface ---------------------------------------------------------------

    EGGS_SDL_CHECK(!window.has_surface());
    auto const acquired = window.get_surface();
    EGGS_SDL_CHECK(acquired.has_value());
    if (!acquired) return eggs::test_support::report();
    EGGS_SDL_CHECK(window.has_surface());

    sdl::surface const surface = *acquired;
    EGGS_SDL_CHECK(surface);
    EGGS_SDL_CHECK(surface.size().width == pixel_size->width);
    EGGS_SDL_CHECK(surface.size().height == pixel_size->height);

    EGGS_SDL_CHECK(surface.fill({10, 20, 30}).has_value());
    EGGS_SDL_CHECK(same(pixel(surface, 0, 0), {10, 20, 30}));
    EGGS_SDL_CHECK(surface.fill({1, 2, 40, 50}, {200, 100, 50}).has_value());
    EGGS_SDL_CHECK(same(pixel(surface, 2, 3), {200, 100, 50}));
    EGGS_SDL_CHECK(same(pixel(surface, 0, 0), {10, 20, 30}));

    EGGS_SDL_CHECK(window.update_surface().has_value());
    std::array<sdl::rect, 2> const rects = {
        sdl::rect{.x = 0, .y = 0, .width = 10, .height = 10},
        sdl::rect{.x = 20, .y = 20, .width = 5, .height = 5}
    };
    EGGS_SDL_CHECK(window.update_surface(rects).has_value());

    EGGS_SDL_CHECK(window.destroy_surface().has_value());
    EGGS_SDL_CHECK(!window.has_surface());

#if SDL_VERSION_ATLEAST(3, 4, 0)
    // -- Progress --------------------------------------------------------------

    EGGS_SDL_CHECK(
        window.set_progress_state(sdl::progress_state::normal).has_value()
    );
    auto const state = window.get_progress_state();
    EGGS_SDL_CHECK(state.has_value());
    EGGS_SDL_CHECK(state && *state == sdl::progress_state::normal);
    EGGS_SDL_CHECK(window.set_progress_value(0.5F).has_value());
    auto const value = window.progress_value();
    EGGS_SDL_CHECK(value.has_value());
    EGGS_SDL_CHECK(value && *value > 0.49F && *value < 0.51F);
    EGGS_SDL_CHECK(
        window.set_progress_state(sdl::progress_state::none).has_value()
    );

    // Only Emscripten acts on it, the other platforms ignore or reject it.
    (void)window.set_fill_document(false);
#endif

    // -- Ownership -------------------------------------------------------------

    sdl::unique_window const moved = std::move(owner);
    EGGS_SDL_CHECK(!owner);
    EGGS_SDL_CHECK(moved);
    EGGS_SDL_CHECK(moved.get() == window);

    return eggs::test_support::report();
}
