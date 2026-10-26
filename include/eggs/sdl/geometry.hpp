// Eggs.SDL
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <eggs/sdl/detail/geometry.hpp> // IWYU pragma: export

#include <cstddef>

#include <SDL3/SDL_rect.h>

namespace eggs::sdl {

// Plain aggregates. fpoint and frect have the layout of SDL_FPoint and
// SDL_FRect, so arrays of them can be handed to SDL as they are.
struct fpoint
{
    float x;
    float y;
};

struct fextent
{
    float width;
    float height;
};

struct frect
{
    float x;
    float y;
    float width;
    float height;
};

static_assert(sizeof(fpoint) == sizeof(SDL_FPoint));
static_assert(offsetof(fpoint, x) == offsetof(SDL_FPoint, x));
static_assert(offsetof(fpoint, y) == offsetof(SDL_FPoint, y));

static_assert(sizeof(frect) == sizeof(SDL_FRect));
static_assert(offsetof(frect, x) == offsetof(SDL_FRect, x));
static_assert(offsetof(frect, y) == offsetof(SDL_FRect, y));
static_assert(offsetof(frect, width) == offsetof(SDL_FRect, w));
static_assert(offsetof(frect, height) == offsetof(SDL_FRect, h));

} // namespace eggs::sdl
