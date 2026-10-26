// Eggs.SDL
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#include "surface_demo.hpp"

// An optional path to a BMP file is the sprite.
int main(int argc, char** argv)
{
    return surface_demo::run({.sprite = argc > 1 ? argv[1] : nullptr});
}
