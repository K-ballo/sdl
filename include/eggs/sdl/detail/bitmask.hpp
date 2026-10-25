// Eggs.SDL
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <type_traits>

namespace eggs::sdl {

template <typename E>
inline constexpr bool enable_bitmask_operators = false;

template <typename E>
concept bitmask_enum = std::is_enum_v<E> && enable_bitmask_operators<E>;

template <bitmask_enum E>
constexpr auto operator|(E lhs, E rhs) noexcept -> E
{
    using U = std::underlying_type_t<E>;
    return static_cast<E>(static_cast<U>(lhs) | static_cast<U>(rhs));
}

template <bitmask_enum E>
constexpr auto operator&(E lhs, E rhs) noexcept -> E
{
    using U = std::underlying_type_t<E>;
    return static_cast<E>(static_cast<U>(lhs) & static_cast<U>(rhs));
}

template <bitmask_enum E>
constexpr auto operator^(E lhs, E rhs) noexcept -> E
{
    using U = std::underlying_type_t<E>;
    return static_cast<E>(static_cast<U>(lhs) ^ static_cast<U>(rhs));
}

template <bitmask_enum E>
constexpr auto operator~(E value) noexcept -> E
{
    using U = std::underlying_type_t<E>;
    return static_cast<E>(~static_cast<U>(value));
}

template <bitmask_enum E>
constexpr auto operator|=(E& lhs, E rhs) noexcept -> E&
{
    return lhs = lhs | rhs;
}

template <bitmask_enum E>
constexpr auto operator&=(E& lhs, E rhs) noexcept -> E&
{
    return lhs = lhs & rhs;
}

template <bitmask_enum E>
constexpr auto operator^=(E& lhs, E rhs) noexcept -> E&
{
    return lhs = lhs ^ rhs;
}

} // namespace eggs::sdl
