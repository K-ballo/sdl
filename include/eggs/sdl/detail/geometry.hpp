// Eggs.SDL
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#pragma once

namespace eggs::sdl {

struct extent
{
    int width;
    int height;
};

struct point
{
    int x;
    int y;
};

struct rect
{
    int x;
    int y;
    int width;
    int height;
};

} // namespace eggs::sdl
