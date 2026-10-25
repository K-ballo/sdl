// Eggs.SDL
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <expected>
#include <string>
#include <string_view>

#include <SDL3/SDL_error.h>

namespace eggs::sdl {

// SDL keeps the error string per thread, so current() must run on the
// thread that made the failing call.
class error
{
  public:
    // Captures SDL_GetError() right now; SDL may overwrite it on the next call.
    [[nodiscard]] static auto current() -> error
    {
        return error(SDL_GetError());
    }

    // Empty when SDL reported a failure without setting an error (see push).
    [[nodiscard]] auto message() const noexcept -> std::string_view
    {
        return message_;
    }

  private:
    explicit error(char const* message)
        : message_(message)
    {
    }

    std::string message_;
};

namespace detail {

[[nodiscard]] inline auto check(bool ok) -> std::expected<void, error>
{
    if (ok) return {};
    return std::unexpected(error::current());
}

} // namespace detail

} // namespace eggs::sdl
