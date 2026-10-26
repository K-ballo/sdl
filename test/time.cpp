// Eggs.SDL
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#include <eggs/sdl.hpp>

#include <chrono>
#include <cstdint>
#include <ratio>
#include <type_traits>

#include "support.hpp"

namespace sdl = eggs::sdl;

// std::chrono::is_clock_v is C++20 and missing from older standard libraries,
// the requirements it checks are also spelled out below.
#if defined(__cpp_lib_chrono) && __cpp_lib_chrono >= 201907L
static_assert(std::chrono::is_clock_v<sdl::clock>);
#endif
static_assert(sdl::clock::is_steady);
static_assert(
    std::is_same_v<decltype(sdl::clock::now()), sdl::clock::time_point>
);
static_assert(std::is_same_v<
              sdl::clock::time_point,
              std::chrono::time_point<sdl::clock, sdl::clock::duration>>);
static_assert(std::is_same_v<sdl::clock::duration, std::chrono::nanoseconds>);
static_assert(std::is_same_v<
              sdl::clock::duration,
              std::chrono::duration<sdl::clock::rep, sdl::clock::period>>);
static_assert(sizeof(sdl::clock::rep) == sizeof(std::int64_t));
static_assert(std::is_same_v<sdl::clock::period, std::nano>);
static_assert(noexcept(sdl::clock::now()));
static_assert(noexcept(sdl::delay(std::chrono::nanoseconds{})));
static_assert(noexcept(sdl::delay_precise(std::chrono::nanoseconds{})));

int main()
{
    using std::chrono::milliseconds;
    using std::chrono::nanoseconds;
    using std::chrono::seconds;

    // -- The clock -------------------------------------------------------------

    auto const before = sdl::clock::now();
    sdl::delay(milliseconds{5});
    auto const after = sdl::clock::now();
    EGGS_SDL_CHECK(after > before);
    EGGS_SDL_CHECK(after - before >= milliseconds{4});
    EGGS_SDL_CHECK(after - before < seconds{2});

    // -- Precise delay ---------------------------------------------------------

    auto const start = sdl::clock::now();
    sdl::delay_precise(milliseconds{5});
    EGGS_SDL_CHECK(sdl::clock::now() - start >= milliseconds{5});

    // A duration that is not positive does not wait.
    auto const quick = sdl::clock::now();
    sdl::delay(-seconds{1});
    sdl::delay_precise(nanoseconds{0});
    sdl::delay_precise(-seconds{1});
    EGGS_SDL_CHECK(sdl::clock::now() - quick < milliseconds{500});

    // -- The counter -----------------------------------------------------------

    EGGS_SDL_CHECK(sdl::performance_frequency() > 0);
    auto const first = sdl::performance_counter();
    sdl::delay(milliseconds{2});
    EGGS_SDL_CHECK(sdl::performance_counter() > first);

    return eggs::test_support::report();
}
