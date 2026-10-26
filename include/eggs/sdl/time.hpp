// Eggs.SDL
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <chrono>
#include <cstdint>

#include <SDL3/SDL_timer.h>

// Everything here may be called from any thread.
namespace eggs::sdl {

// A steady clock that counts the nanoseconds since SDL was initialized. It
// satisfies std::chrono::is_clock. rep and period are those of duration, which
// is std::int64_t and std::nano wherever that is what nanoseconds is made of.
struct clock
{
    using duration = std::chrono::nanoseconds;
    using rep = duration::rep;
    using period = duration::period;
    using time_point = std::chrono::time_point<clock>;
    static constexpr bool is_steady = true;

    [[nodiscard]] static auto now() noexcept -> time_point
    {
        return time_point(duration(static_cast<rep>(SDL_GetTicksNS())));
    }
};

// A counter of platform specific ticks, in counts per second of
// performance_frequency. Unrelated to clock.
[[nodiscard]] inline auto performance_counter() noexcept -> std::uint64_t
{
    return SDL_GetPerformanceCounter();
}

[[nodiscard]] inline auto performance_frequency() noexcept -> std::uint64_t
{
    return SDL_GetPerformanceFrequency();
}

// Waits at least the duration, the precision is that of the operating system.
// A duration that is not positive does not wait. delay_precise spends the
// final stretch spinning to be more accurate, and so costs more processor time.
inline void delay(std::chrono::nanoseconds duration) noexcept
{
    SDL_DelayNS(
        static_cast<std::uint64_t>(duration.count() > 0 ? duration.count() : 0)
    );
}

inline void delay_precise(std::chrono::nanoseconds duration) noexcept
{
    SDL_DelayPrecise(
        static_cast<std::uint64_t>(duration.count() > 0 ? duration.count() : 0)
    );
}

} // namespace eggs::sdl
