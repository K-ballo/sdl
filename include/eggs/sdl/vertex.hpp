// Eggs.SDL
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <eggs/sdl/color.hpp>
#include <eggs/sdl/geometry.hpp>

#include <cstddef>

#include <SDL3/SDL_render.h>

namespace eggs::sdl {

// A plain aggregate with the layout of SDL_Vertex, so that spans of it are
// handed to SDL as they are. The position is in render coordinates, the
// tex_coord in normalized texture coordinates.
struct vertex
{
    fpoint position;
    fcolor color;
    fpoint tex_coord;
};

static_assert(sizeof(vertex) == sizeof(SDL_Vertex));
static_assert(offsetof(vertex, position) == offsetof(SDL_Vertex, position));
static_assert(offsetof(vertex, color) == offsetof(SDL_Vertex, color));
static_assert(offsetof(vertex, tex_coord) == offsetof(SDL_Vertex, tex_coord));

} // namespace eggs::sdl
