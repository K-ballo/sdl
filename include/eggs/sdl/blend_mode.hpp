// Eggs.SDL
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <cstdint>

#include <SDL3/SDL_blendmode.h>

namespace eggs::sdl {

// SDL's values. SDL can also compose custom blend modes, which are values that
// are not enumerators here, so a blend_mode read back from SDL may hold one.
enum class blend_mode : std::uint32_t
{
    none = SDL_BLENDMODE_NONE,
    blend = SDL_BLENDMODE_BLEND,
    blend_premultiplied = SDL_BLENDMODE_BLEND_PREMULTIPLIED,
    add = SDL_BLENDMODE_ADD,
    add_premultiplied = SDL_BLENDMODE_ADD_PREMULTIPLIED,
    mod = SDL_BLENDMODE_MOD,
    mul = SDL_BLENDMODE_MUL,
};

} // namespace eggs::sdl
