// Eggs.SDL
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <cstddef>
#include <cstdint>

#include <SDL3/SDL_pixels.h>

namespace eggs::sdl {

struct rgba;

struct rgb
{
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;

    // Opaque. A conversion function and not a constructor of rgba, which
    // keeps rgba an aggregate.
    constexpr operator rgba() const noexcept;
};

// A plain aggregate with the layout of SDL_Color.
struct rgba
{
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
    std::uint8_t a = 255;
};

constexpr rgb::operator rgba() const noexcept
{
    return {.r = r, .g = g, .b = b};
}

// A plain aggregate with the layout of SDL_FColor.
struct fcolor
{
    float r;
    float g;
    float b;
    float a = 1.0F;
};

static_assert(sizeof(rgba) == sizeof(SDL_Color));
static_assert(offsetof(rgba, r) == offsetof(SDL_Color, r));
static_assert(offsetof(rgba, g) == offsetof(SDL_Color, g));
static_assert(offsetof(rgba, b) == offsetof(SDL_Color, b));
static_assert(offsetof(rgba, a) == offsetof(SDL_Color, a));

static_assert(sizeof(fcolor) == sizeof(SDL_FColor));
static_assert(offsetof(fcolor, r) == offsetof(SDL_FColor, r));
static_assert(offsetof(fcolor, g) == offsetof(SDL_FColor, g));
static_assert(offsetof(fcolor, b) == offsetof(SDL_FColor, b));
static_assert(offsetof(fcolor, a) == offsetof(SDL_FColor, a));

} // namespace eggs::sdl
