// Eggs.SDL
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <eggs/sdl/detail/bitmask.hpp>
#include <eggs/sdl/detail/geometry.hpp> // IWYU pragma: export
#include <eggs/sdl/error.hpp>
#include <eggs/sdl/surface.hpp>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory>
#include <span>
#include <string_view>

#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_video.h>

namespace eggs::sdl {

enum class window_flags : std::uint64_t
{
    none = 0,
    fullscreen = SDL_WINDOW_FULLSCREEN,
    opengl = SDL_WINDOW_OPENGL,
    occluded = SDL_WINDOW_OCCLUDED,
    hidden = SDL_WINDOW_HIDDEN,
    borderless = SDL_WINDOW_BORDERLESS,
    resizable = SDL_WINDOW_RESIZABLE,
    minimized = SDL_WINDOW_MINIMIZED,
    maximized = SDL_WINDOW_MAXIMIZED,
    mouse_grabbed = SDL_WINDOW_MOUSE_GRABBED,
    input_focus = SDL_WINDOW_INPUT_FOCUS,
    mouse_focus = SDL_WINDOW_MOUSE_FOCUS,
    external = SDL_WINDOW_EXTERNAL,
    modal = SDL_WINDOW_MODAL,
    high_pixel_density = SDL_WINDOW_HIGH_PIXEL_DENSITY,
    mouse_capture = SDL_WINDOW_MOUSE_CAPTURE,
    mouse_relative_mode = SDL_WINDOW_MOUSE_RELATIVE_MODE,
    always_on_top = SDL_WINDOW_ALWAYS_ON_TOP,
    utility = SDL_WINDOW_UTILITY,
    tooltip = SDL_WINDOW_TOOLTIP,
    popup_menu = SDL_WINDOW_POPUP_MENU,
    keyboard_grabbed = SDL_WINDOW_KEYBOARD_GRABBED,
    vulkan = SDL_WINDOW_VULKAN,
    metal = SDL_WINDOW_METAL,
    transparent = SDL_WINDOW_TRANSPARENT,
    not_focusable = SDL_WINDOW_NOT_FOCUSABLE,
};

template <>
inline constexpr bool enable_bitmask_operators<window_flags> = true;

// 0 is invalid.
enum class window_id : std::uint32_t
{
};

class window;

struct window_deleter
{
    using pointer = window;

    void operator()(window owned) const noexcept;
};

using unique_window = std::unique_ptr<window, window_deleter>;

// Window functions must be called from the thread that initialized video,
// usually the main thread. A window must outlive the surfaces borrowed from it,
// see get_surface.
class window
{
  public:
    constexpr window() noexcept = default;

    constexpr window(std::nullptr_t) noexcept {}

    explicit constexpr window(SDL_Window* handle) noexcept
        : handle_(handle)
    {
    }

    // Initialize video with init first: SDL initializes it on demand when it
    // is not, and then nothing releases it.
    [[nodiscard]] static auto create(
        char const* title, int width, int height,
        window_flags flags = window_flags::none
    ) -> std::expected<unique_window, error>;

    // Borrowed: the result does not own the window and is valid for as long as
    // its owner is.
    [[nodiscard]] static auto from_id(window_id id)
        -> std::expected<window, error>;

    [[nodiscard]] constexpr auto handle() const noexcept -> SDL_Window*
    {
        return handle_;
    }

    explicit constexpr operator bool() const noexcept
    {
        return handle_ != nullptr;
    }

    constexpr auto operator->() const noexcept -> window const* { return this; }

    friend constexpr auto operator==(window, window) noexcept -> bool = default;

    [[nodiscard]] auto id() const -> std::expected<window_id, error>;

    // SDL-owned, valid until the next set_title or until the window is
    // destroyed. Empty on error.
    [[nodiscard]] auto title() const noexcept -> std::string_view;
    [[nodiscard]] auto set_title(char const* title) const
        -> std::expected<void, error>;

    [[nodiscard]] auto size() const -> std::expected<extent, error>;
    [[nodiscard]] auto set_size(extent size) const
        -> std::expected<void, error>;
    [[nodiscard]] auto pixel_size() const -> std::expected<extent, error>;

    [[nodiscard]] auto show() const -> std::expected<void, error>;
    [[nodiscard]] auto hide() const -> std::expected<void, error>;
    [[nodiscard]] auto raise() const -> std::expected<void, error>;

    // Borrowed from the window, not owned: it becomes invalid after
    // destroy_surface, after the window is destroyed, and after the window is
    // resized or its pixel size changes. Re-acquire it on
    // window_resized_event, window_pixel_size_changed_event and
    // window_exposed_event. Fails for windows created with a 3D API flag.
    [[nodiscard]] auto get_surface() const -> std::expected<surface, error>;
    [[nodiscard]] auto has_surface() const noexcept -> bool;

    [[nodiscard]] auto update_surface() const -> std::expected<void, error>;
    [[nodiscard]] auto update_surface(std::span<rect const> rects) const
        -> std::expected<void, error>;

    // Invalidates any surface previously returned by get_surface.
    [[nodiscard]] auto destroy_surface() const -> std::expected<void, error>;

  private:
    SDL_Window* handle_ = nullptr;
};

inline void window_deleter::operator()(window owned) const noexcept
{
    SDL_DestroyWindow(owned.handle());
}

inline auto
window::create(char const* title, int width, int height, window_flags flags)
    -> std::expected<unique_window, error>
{
    SDL_Window* const created = SDL_CreateWindow(
        title, width, height, static_cast<SDL_WindowFlags>(flags)
    );
    if (created == nullptr) return std::unexpected(error::current());
    return unique_window(window(created));
}

inline auto window::from_id(window_id id) -> std::expected<window, error>
{
    SDL_Window* const found =
        SDL_GetWindowFromID(static_cast<SDL_WindowID>(id));
    if (found == nullptr) return std::unexpected(error::current());
    return window(found);
}

inline auto window::id() const -> std::expected<window_id, error>
{
    assert(handle_);
    SDL_WindowID const value = SDL_GetWindowID(handle_);
    if (value == 0) return std::unexpected(error::current());
    return static_cast<window_id>(value);
}

inline auto window::title() const noexcept -> std::string_view
{
    assert(handle_);
    char const* const value = SDL_GetWindowTitle(handle_);
    return value != nullptr ? std::string_view(value) : std::string_view();
}

inline auto window::set_title(char const* title) const
    -> std::expected<void, error>
{
    assert(handle_);
    return detail::check(SDL_SetWindowTitle(handle_, title));
}

inline auto window::size() const -> std::expected<extent, error>
{
    assert(handle_);
    extent result{};
    if (!SDL_GetWindowSize(handle_, &result.width, &result.height))
        return std::unexpected(error::current());
    return result;
}

inline auto window::set_size(extent size) const -> std::expected<void, error>
{
    assert(handle_);
    return detail::check(SDL_SetWindowSize(handle_, size.width, size.height));
}

inline auto window::pixel_size() const -> std::expected<extent, error>
{
    assert(handle_);
    extent result{};
    if (!SDL_GetWindowSizeInPixels(handle_, &result.width, &result.height))
        return std::unexpected(error::current());
    return result;
}

inline auto window::show() const -> std::expected<void, error>
{
    assert(handle_);
    return detail::check(SDL_ShowWindow(handle_));
}

inline auto window::hide() const -> std::expected<void, error>
{
    assert(handle_);
    return detail::check(SDL_HideWindow(handle_));
}

inline auto window::raise() const -> std::expected<void, error>
{
    assert(handle_);
    return detail::check(SDL_RaiseWindow(handle_));
}

inline auto window::get_surface() const -> std::expected<surface, error>
{
    assert(handle_);
    SDL_Surface* const acquired = SDL_GetWindowSurface(handle_);
    if (acquired == nullptr) return std::unexpected(error::current());
    return surface(acquired);
}

inline auto window::has_surface() const noexcept -> bool
{
    assert(handle_);
    return SDL_WindowHasSurface(handle_);
}

inline auto window::update_surface() const -> std::expected<void, error>
{
    assert(handle_);
    return detail::check(SDL_UpdateWindowSurface(handle_));
}

inline auto window::update_surface(std::span<rect const> rects) const
    -> std::expected<void, error>
{
    assert(handle_);
    static_assert(sizeof(rect) == sizeof(SDL_Rect));
    static_assert(offsetof(rect, x) == offsetof(SDL_Rect, x));
    static_assert(offsetof(rect, y) == offsetof(SDL_Rect, y));
    static_assert(offsetof(rect, width) == offsetof(SDL_Rect, w));
    static_assert(offsetof(rect, height) == offsetof(SDL_Rect, h));
    // The layouts are identical, converting would need an allocation.
    return detail::check(SDL_UpdateWindowSurfaceRects(
        handle_, reinterpret_cast<SDL_Rect const*>(rects.data()),
        static_cast<int>(rects.size())
    ));
}

inline auto window::destroy_surface() const -> std::expected<void, error>
{
    assert(handle_);
    return detail::check(SDL_DestroyWindowSurface(handle_));
}

} // namespace eggs::sdl
