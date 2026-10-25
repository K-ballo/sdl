// Eggs.SDL
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <eggs/sdl/detail/geometry.hpp>
#include <eggs/sdl/error.hpp>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <expected>

#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_surface.h>

namespace eggs::sdl {

struct rgb
{
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
};

// In stage 1 only ever the borrowed window surface, see window::get_surface
// for when it becomes invalid. Use it from the thread that owns the window.
class surface
{
  public:
    constexpr surface() noexcept = default;

    constexpr surface(std::nullptr_t) noexcept {}

    explicit constexpr surface(SDL_Surface* handle) noexcept
        : handle_(handle)
    {
    }

    [[nodiscard]] constexpr auto handle() const noexcept -> SDL_Surface*
    {
        return handle_;
    }

    explicit constexpr operator bool() const noexcept
    {
        return handle_ != nullptr;
    }

    friend constexpr auto operator==(surface, surface) noexcept
        -> bool = default;

    [[nodiscard]] auto size() const noexcept -> extent
    {
        assert(handle_);
        return {.width = handle_->w, .height = handle_->h};
    }

    [[nodiscard]] auto fill(rgb color) const -> std::expected<void, error>
    {
        assert(handle_);
        return detail::check(SDL_FillSurfaceRect(handle_, nullptr, map(color)));
    }

    [[nodiscard]] auto fill(rect area, rgb color) const
        -> std::expected<void, error>
    {
        assert(handle_);
        SDL_Rect const sdl_area = {
            .x = area.x, .y = area.y, .w = area.width, .h = area.height
        };
        return detail::check(
            SDL_FillSurfaceRect(handle_, &sdl_area, map(color))
        );
    }

  private:
    [[nodiscard]] auto map(rgb color) const noexcept -> std::uint32_t
    {
        return SDL_MapSurfaceRGB(handle_, color.r, color.g, color.b);
    }

    SDL_Surface* handle_ = nullptr;
};

} // namespace eggs::sdl
