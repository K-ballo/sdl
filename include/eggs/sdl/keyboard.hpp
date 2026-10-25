// Eggs.SDL
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <eggs/sdl/detail/bitmask.hpp>
#include <eggs/sdl/detail/keycode.hpp>  // IWYU pragma: export
#include <eggs/sdl/detail/scancode.hpp> // IWYU pragma: export

#include <cstdint>
#include <string_view>

#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_scancode.h>

namespace eggs::sdl {

enum class key_modifiers : std::uint16_t
{
    none = SDL_KMOD_NONE,
    left_shift = SDL_KMOD_LSHIFT,
    right_shift = SDL_KMOD_RSHIFT,
    level5 = SDL_KMOD_LEVEL5,
    left_ctrl = SDL_KMOD_LCTRL,
    right_ctrl = SDL_KMOD_RCTRL,
    left_alt = SDL_KMOD_LALT,
    right_alt = SDL_KMOD_RALT,
    left_gui = SDL_KMOD_LGUI,
    right_gui = SDL_KMOD_RGUI,
    num = SDL_KMOD_NUM,
    caps = SDL_KMOD_CAPS,
    mode = SDL_KMOD_MODE,
    scroll = SDL_KMOD_SCROLL,
    ctrl = SDL_KMOD_CTRL,
    shift = SDL_KMOD_SHIFT,
    alt = SDL_KMOD_ALT,
    gui = SDL_KMOD_GUI,
};

template <>
inline constexpr bool enable_bitmask_operators<key_modifiers> = true;

// SDL-owned string, valid until the next call to SDL_SetScancodeName. Empty
// when the scancode has no name.
[[nodiscard]] inline auto name(scancode code) noexcept -> std::string_view
{
    return SDL_GetScancodeName(static_cast<SDL_Scancode>(code));
}

} // namespace eggs::sdl
