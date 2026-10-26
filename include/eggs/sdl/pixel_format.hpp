// Eggs.SDL
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <eggs/sdl/detail/pixel_format_types.hpp> // IWYU pragma: export

#include <cstdint>
#include <string_view>

#include <SDL3/SDL_pixels.h>

namespace eggs::sdl {

// Every SDL pixel format with SDL's value, in snake case without the prefix,
// including the endian-dependent aliases such as rgba32. mjpg needs SDL 3.4.0.
enum class pixel_format : std::uint32_t
{
#define EGGS_SDL_PIXEL_FORMAT_ENUMERATOR(name, value) name = (value),
    EGGS_SDL_PIXEL_FORMATS(EGGS_SDL_PIXEL_FORMAT_ENUMERATOR)
#undef EGGS_SDL_PIXEL_FORMAT_ENUMERATOR
};

// SDL-owned and static, "SDL_PIXELFORMAT_UNKNOWN" for a value that is not one
// of the pixel formats.
[[nodiscard]] inline auto name(pixel_format format) noexcept -> std::string_view
{
    return SDL_GetPixelFormatName(static_cast<SDL_PixelFormat>(format));
}

// The FourCC formats, which are the YUV ones, have 0 bits per pixel and a
// best effort 1 or 2 bytes per pixel, as SDL defines them.
[[nodiscard]] constexpr auto bits_per_pixel(pixel_format format) noexcept -> int
{
    auto const value = static_cast<SDL_PixelFormat>(format);
    return SDL_BITSPERPIXEL(value);
}

[[nodiscard]] constexpr auto bytes_per_pixel(pixel_format format) noexcept
    -> int
{
    auto const value = static_cast<SDL_PixelFormat>(format);
    return SDL_BYTESPERPIXEL(value);
}

[[nodiscard]] constexpr auto has_alpha(pixel_format format) noexcept -> bool
{
    auto const value = static_cast<SDL_PixelFormat>(format);
    return SDL_ISPIXELFORMAT_ALPHA(value);
}

} // namespace eggs::sdl
