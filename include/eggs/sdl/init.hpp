// Eggs.SDL
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <eggs/sdl/detail/bitmask.hpp>
#include <eggs/sdl/error.hpp>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory>

#include <SDL3/SDL_init.h>

namespace eggs::sdl {

// Video implies events and gamepad implies joystick, SDL initializes the
// implied subsystems as well and the owner releases them together.
enum class init_flags : std::uint32_t
{
    none = 0,
    audio = SDL_INIT_AUDIO,
    video = SDL_INIT_VIDEO,
    joystick = SDL_INIT_JOYSTICK,
    haptic = SDL_INIT_HAPTIC,
    gamepad = SDL_INIT_GAMEPAD,
    events = SDL_INIT_EVENTS,
    sensor = SDL_INIT_SENSOR,
    camera = SDL_INIT_CAMERA,
};

template <>
inline constexpr bool enable_bitmask_operators<init_flags> = true;

// The flags an owner initialized. Pointer-sized so that the owner is.
class subsystems
{
  public:
    constexpr subsystems() noexcept = default;

    constexpr subsystems(std::nullptr_t) noexcept {}

    explicit constexpr subsystems(init_flags flags) noexcept
        : bits_(static_cast<std::uintptr_t>(flags))
    {
    }

    [[nodiscard]] constexpr auto flags() const noexcept -> init_flags
    {
        return static_cast<init_flags>(bits_);
    }

    explicit constexpr operator bool() const noexcept { return bits_ != 0; }

    constexpr auto operator->() const noexcept -> subsystems const*
    {
        return this;
    }

    friend constexpr auto operator==(subsystems, subsystems) noexcept
        -> bool = default;

  private:
    std::uintptr_t bits_ = 0;
};

struct subsystems_deleter
{
    using pointer = subsystems;

    void operator()(subsystems owned) const noexcept
    {
        SDL_QuitSubSystem(static_cast<SDL_InitFlags>(owned.flags()));
    }
};

// Quits only the subsystems it initialized (SDL_QuitSubSystem, never SDL_Quit).
using unique_subsystems = std::unique_ptr<subsystems, subsystems_deleter>;

// Video, and event pumping, must be used from the thread that initialized
// video, usually the main thread. SDL refcounts subsystems, so nested calls are
// safe. Precondition: flags is not empty.
[[nodiscard]] inline auto init(init_flags flags)
    -> std::expected<unique_subsystems, error>
{
    assert(flags != init_flags::none);
    if (!SDL_InitSubSystem(static_cast<SDL_InitFlags>(flags)))
        return std::unexpected(error::current());
    return unique_subsystems(subsystems(flags));
}

// Escape hatch: SDL_Quit tears everything down, owners are not aware of it.
inline void quit() noexcept
{
    SDL_Quit();
}

// Subset of flags currently initialized; all initialized ones when empty.
[[nodiscard]] inline auto was_init(init_flags flags = init_flags::none) noexcept
    -> init_flags
{
    return static_cast<init_flags>(
        SDL_WasInit(static_cast<SDL_InitFlags>(flags))
    );
}

[[nodiscard]] inline auto
set_app_metadata(char const* name, char const* version, char const* identifier)
    -> std::expected<void, error>
{
    return detail::check(SDL_SetAppMetadata(name, version, identifier));
}

[[nodiscard]] inline auto
set_app_metadata_property(char const* name, char const* value)
    -> std::expected<void, error>
{
    return detail::check(SDL_SetAppMetadataProperty(name, value));
}

} // namespace eggs::sdl
