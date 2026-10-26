// Eggs.SDL
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <eggs/sdl/blend_mode.hpp>
#include <eggs/sdl/color.hpp> // IWYU pragma: export
#include <eggs/sdl/detail/geometry.hpp>
#include <eggs/sdl/error.hpp>
#include <eggs/sdl/pixel_format.hpp>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory>
#include <optional>
#include <span>
#include <utility>

#include <SDL3/SDL_blendmode.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_version.h>

namespace eggs::sdl {

// SDL's values. pixelart needs SDL 3.4.0.
enum class scale_mode : std::int32_t
{
    nearest = SDL_SCALEMODE_NEAREST,
    linear = SDL_SCALEMODE_LINEAR,
#if SDL_VERSION_ATLEAST(3, 4, 0)
    pixelart = SDL_SCALEMODE_PIXELART,
#endif
};

// SDL's values. horizontal_and_vertical needs SDL 3.4.0.
enum class flip_mode : std::int32_t
{
    none = SDL_FLIP_NONE,
    horizontal = SDL_FLIP_HORIZONTAL,
    vertical = SDL_FLIP_VERTICAL,
#if SDL_VERSION_ATLEAST(3, 4, 0)
    horizontal_and_vertical = SDL_FLIP_HORIZONTAL_AND_VERTICAL,
#endif
};

class surface;
class surface_lock;

struct surface_deleter
{
    using pointer = surface;

    void operator()(surface owned) const noexcept;
};

// Destroys the surface, which no other thread may be using.
using unique_surface = std::unique_ptr<surface, surface_deleter>;

// A handle to a pixel surface. It is either one that the caller owns, through
// unique_surface, or a borrowed one such as window::get_surface, whose validity
// is then that of its owner. Never put a borrowed surface in a unique_surface.
// Every member works on both, and the window surface keeps the validity rules
// of get_surface.
//
// The functions that create or load a surface may be called from any thread.
// A surface is used by one thread at a time, and blit and blit_scaled need
// exclusive use of both surfaces. Surfaces that are different may be used on
// different threads.
class surface
{
  public:
    constexpr surface() noexcept = default;

    constexpr surface(std::nullptr_t) noexcept {}

    explicit constexpr surface(SDL_Surface* handle) noexcept
        : handle_(handle)
    {
    }

    // A new surface of the size, zero filled. A negative size fails.
    [[nodiscard]] static auto create(extent size, pixel_format format)
        -> std::expected<unique_surface, error>;

    // Wraps the caller's memory, nothing is copied: the memory must outlive the
    // surface, and must not be accessed except through a surface_lock while
    // the surface uses it. The pitch is the number of bytes between rows.
    // Fails when the span is smaller than the pixels, which for the FourCC
    // formats is only checked for their first plane: a planar format needs
    // more, and that is the caller's to provide.
    [[nodiscard]] static auto from_pixels(
        extent size, pixel_format format, std::span<std::byte> pixels, int pitch
    ) -> std::expected<unique_surface, error>;

    [[nodiscard]] static auto load_bmp(char const* path)
        -> std::expected<unique_surface, error>;

    [[nodiscard]] constexpr auto handle() const noexcept -> SDL_Surface*
    {
        return handle_;
    }

    explicit constexpr operator bool() const noexcept
    {
        return handle_ != nullptr;
    }

    constexpr auto operator->() const noexcept -> surface const*
    {
        return this;
    }

    friend constexpr auto operator==(surface, surface) noexcept
        -> bool = default;

    // -- Queries

    [[nodiscard]] auto size() const noexcept -> extent;
    [[nodiscard]] auto format() const noexcept -> pixel_format;
    [[nodiscard]] auto pitch() const noexcept -> int;

    // -- New surfaces, owned by the caller

    // These return a new surface and never modify this one.
    [[nodiscard]] auto duplicate() const
        -> std::expected<unique_surface, error>;
    [[nodiscard]] auto convert(pixel_format format) const
        -> std::expected<unique_surface, error>;
    [[nodiscard]] auto scale(extent size, scale_mode mode) const
        -> std::expected<unique_surface, error>;

    // -- In place

    [[nodiscard]] auto flip(flip_mode mode) const -> std::expected<void, error>;

    // The linear flag converts to linear space to multiply, and back.
    [[nodiscard]] auto premultiply_alpha(bool linear = false) const
        -> std::expected<void, error>;

    // -- Drawing

    // Sets every pixel, the clip rect does not apply. The float color is
    // converted to the format of the surface.
    [[nodiscard]] auto clear(fcolor color) const -> std::expected<void, error>;

    // Sets the pixels to the color as the format of the surface represents it,
    // alpha too, no blending takes place. The clip rect applies.
    [[nodiscard]] auto fill(rgba color) const -> std::expected<void, error>;
    [[nodiscard]] auto fill(rect area, rgba color) const
        -> std::expected<void, error>;
    [[nodiscard]] auto fill(std::span<rect const> areas, rgba color) const
        -> std::expected<void, error>;

    // Copies this surface, the source, onto the destination, with the blend
    // mode, color and alpha modulation and color key of the source, and the
    // clip rect of the destination. Coordinates are in destination pixels.
    // Only the position of the destination is used, the size comes from the
    // source, which is the whole surface unless a source rect says otherwise.
    //
    // The surfaces may have different pixel formats and SDL converts as it
    // goes, but converting this surface once to the format of the destination
    // with convert is faster when it is blitted repeatedly.
    [[nodiscard]] auto blit(
        surface destination, point at = {}, std::optional<rect> source = {}
    ) const -> std::expected<void, error>;

    // As blit, stretching the source to fill the area of the destination.
    [[nodiscard]] auto blit_scaled(
        surface destination, rect area, scale_mode mode,
        std::optional<rect> source = {}
    ) const -> std::expected<void, error>;

    // -- Blit state, as SDL defines for a surface that is the source or the
    // destination of a blit.

    // The key is converted to the format of the surface, a pixel of the source
    // that has it is then skipped. Empty clears the key.
    [[nodiscard]] auto set_color_key(std::optional<rgba> key) const
        -> std::expected<void, error>;

    // Empty when there is no key. The key as the format of the surface holds
    // it, which is the color that was set only as far as the format can
    // represent it.
    [[nodiscard]] auto color_key() const noexcept -> std::optional<rgba>;

    [[nodiscard]] auto set_color_mod(rgb color) const
        -> std::expected<void, error>;
    [[nodiscard]] auto color_mod() const -> std::expected<rgb, error>;
    [[nodiscard]] auto set_alpha_mod(std::uint8_t alpha) const
        -> std::expected<void, error>;
    [[nodiscard]] auto alpha_mod() const -> std::expected<std::uint8_t, error>;
    [[nodiscard]] auto set_blend_mode(blend_mode mode) const
        -> std::expected<void, error>;
    [[nodiscard]] auto get_blend_mode() const
        -> std::expected<blend_mode, error>;

    // Restricts what blits and fills draw to the area, empty clears it. SDL
    // clips the area to the surface. Returns whether the area intersects the
    // surface: false is not an error, SDL then clips every blit completely,
    // and it does not set an error for the caller to read.
    [[nodiscard]] auto set_clip_rect(std::optional<rect> area) const -> bool;
    [[nodiscard]] auto clip_rect() const -> std::expected<rect, error>;

    // -- Single pixels, slow, for tools and tests

    [[nodiscard]] auto read_pixel(point at) const -> std::expected<rgba, error>;
    [[nodiscard]] auto write_pixel(point at, rgba color) const
        -> std::expected<void, error>;

    // -- Raw pixel access and files

    // Pixel memory may be accessed only while the surface_lock is alive: its
    // span is invalid once the lock or the surface is gone. Unlock by
    // destroying the lock, before the surface is used for anything else.
    [[nodiscard]] auto lock() const -> std::expected<surface_lock, error>;

    [[nodiscard]] auto save_bmp(char const* path) const
        -> std::expected<void, error>;

  private:
    SDL_Surface* handle_ = nullptr;
};

// Unlocks the surface when destroyed. The surface must outlive it.
class surface_lock
{
  public:
    surface_lock(surface_lock const&) = delete;
    auto operator=(surface_lock const&) -> surface_lock& = delete;

    surface_lock(surface_lock&& other) noexcept
        : surface_(std::exchange(other.surface_, nullptr))
    {
    }

    auto operator=(surface_lock&& other) noexcept -> surface_lock&
    {
        if (this != &other) {
            release();
            surface_ = std::exchange(other.surface_, nullptr);
        }
        return *this;
    }

    ~surface_lock() { release(); }

    // The pitch times the height bytes: the pixels of the surface, and for a
    // FourCC format its first plane. Empty once the lock was moved from.
    [[nodiscard]] auto pixels() const noexcept -> std::span<std::byte>
    {
        if (surface_ == nullptr || surface_->pixels == nullptr) return {};
        return {
            static_cast<std::byte*>(surface_->pixels),
            static_cast<std::size_t>(surface_->pitch) *
                static_cast<std::size_t>(surface_->h)
        };
    }

    // The number of bytes between rows, 0 once the lock was moved from.
    [[nodiscard]] auto pitch() const noexcept -> int
    {
        return surface_ != nullptr ? surface_->pitch : 0;
    }

  private:
    friend class surface;

    explicit surface_lock(SDL_Surface* locked) noexcept
        : surface_(locked)
    {
    }

    void release() noexcept
    {
        if (surface_ != nullptr) SDL_UnlockSurface(surface_);
        surface_ = nullptr;
    }

    SDL_Surface* surface_ = nullptr;
};

namespace detail {

[[nodiscard]] constexpr auto to_sdl(rect value) noexcept -> SDL_Rect
{
    return {.x = value.x, .y = value.y, .w = value.width, .h = value.height};
}

[[nodiscard]] inline auto owned(SDL_Surface* created)
    -> std::expected<unique_surface, error>
{
    if (created == nullptr) return std::unexpected(error::current());
    return unique_surface(surface(created));
}

// The smallest number of bytes that can hold the rows, the pitch apart. The
// FourCC formats have no bits per pixel, the first plane is what is counted.
[[nodiscard]] inline auto
minimum_size(extent size, pixel_format format, int pitch) noexcept
    -> std::size_t
{
    auto const rows = static_cast<std::size_t>(size.height);
    auto const stride = static_cast<std::size_t>(pitch);
    int const bits = bits_per_pixel(format);
    if (bits == 0) return rows * stride;
    auto const row = ((static_cast<std::size_t>(size.width) *
                       static_cast<std::size_t>(bits)) +
                      7) /
                     8;
    return ((rows - 1) * stride) + row;
}

} // namespace detail

inline void surface_deleter::operator()(surface owned) const noexcept
{
    SDL_DestroySurface(owned.handle());
}

inline auto surface::create(extent size, pixel_format format)
    -> std::expected<unique_surface, error>
{
    return detail::owned(SDL_CreateSurface(
        size.width, size.height, static_cast<SDL_PixelFormat>(format)
    ));
}

inline auto surface::from_pixels(
    extent size, pixel_format format, std::span<std::byte> pixels, int pitch
) -> std::expected<unique_surface, error>
{
    // SDL validates the rest, this is only what it cannot see.
    if (size.width > 0 && size.height > 0 && pitch > 0 &&
        pixels.size() < detail::minimum_size(size, format, pitch)) {
        (void)SDL_SetError("The pixel buffer is smaller than the surface");
        return std::unexpected(error::current());
    }
    return detail::owned(SDL_CreateSurfaceFrom(
        size.width, size.height, static_cast<SDL_PixelFormat>(format),
        pixels.data(), pitch
    ));
}

inline auto surface::load_bmp(char const* path)
    -> std::expected<unique_surface, error>
{
    return detail::owned(SDL_LoadBMP(path));
}

inline auto surface::size() const noexcept -> extent
{
    assert(handle_);
    return {.width = handle_->w, .height = handle_->h};
}

inline auto surface::format() const noexcept -> pixel_format
{
    assert(handle_);
    return static_cast<pixel_format>(handle_->format);
}

inline auto surface::pitch() const noexcept -> int
{
    assert(handle_);
    return handle_->pitch;
}

inline auto surface::duplicate() const -> std::expected<unique_surface, error>
{
    assert(handle_);
    return detail::owned(SDL_DuplicateSurface(handle_));
}

inline auto surface::convert(pixel_format format) const
    -> std::expected<unique_surface, error>
{
    assert(handle_);
    return detail::owned(
        SDL_ConvertSurface(handle_, static_cast<SDL_PixelFormat>(format))
    );
}

inline auto surface::scale(extent size, scale_mode mode) const
    -> std::expected<unique_surface, error>
{
    assert(handle_);
    return detail::owned(SDL_ScaleSurface(
        handle_, size.width, size.height, static_cast<SDL_ScaleMode>(mode)
    ));
}

inline auto surface::flip(flip_mode mode) const -> std::expected<void, error>
{
    assert(handle_);
    return detail::check(
        SDL_FlipSurface(handle_, static_cast<SDL_FlipMode>(mode))
    );
}

inline auto surface::premultiply_alpha(bool linear) const
    -> std::expected<void, error>
{
    assert(handle_);
    return detail::check(SDL_PremultiplySurfaceAlpha(handle_, linear));
}

inline auto surface::clear(fcolor color) const -> std::expected<void, error>
{
    assert(handle_);
    return detail::check(
        SDL_ClearSurface(handle_, color.r, color.g, color.b, color.a)
    );
}

inline auto surface::fill(rgba color) const -> std::expected<void, error>
{
    assert(handle_);
    return detail::check(SDL_FillSurfaceRect(
        handle_, nullptr,
        SDL_MapSurfaceRGBA(handle_, color.r, color.g, color.b, color.a)
    ));
}

inline auto surface::fill(rect area, rgba color) const
    -> std::expected<void, error>
{
    assert(handle_);
    SDL_Rect const sdl_area = detail::to_sdl(area);
    return detail::check(SDL_FillSurfaceRect(
        handle_, &sdl_area,
        SDL_MapSurfaceRGBA(handle_, color.r, color.g, color.b, color.a)
    ));
}

inline auto surface::fill(std::span<rect const> areas, rgba color) const
    -> std::expected<void, error>
{
    assert(handle_);
    static_assert(sizeof(rect) == sizeof(SDL_Rect));
    static_assert(offsetof(rect, x) == offsetof(SDL_Rect, x));
    static_assert(offsetof(rect, y) == offsetof(SDL_Rect, y));
    static_assert(offsetof(rect, width) == offsetof(SDL_Rect, w));
    static_assert(offsetof(rect, height) == offsetof(SDL_Rect, h));
    // The layouts are identical, converting would need an allocation.
    return detail::check(SDL_FillSurfaceRects(
        handle_, reinterpret_cast<SDL_Rect const*>(areas.data()),
        static_cast<int>(areas.size()),
        SDL_MapSurfaceRGBA(handle_, color.r, color.g, color.b, color.a)
    ));
}

inline auto
surface::blit(surface destination, point at, std::optional<rect> source) const
    -> std::expected<void, error>
{
    assert(handle_);
    assert(destination);
    SDL_Rect const sdl_source = detail::to_sdl(source.value_or(rect{}));
    SDL_Rect const sdl_destination = {.x = at.x, .y = at.y, .w = 0, .h = 0};
    return detail::check(SDL_BlitSurface(
        handle_, source ? &sdl_source : nullptr, destination.handle(),
        &sdl_destination
    ));
}

inline auto surface::blit_scaled(
    surface destination, rect area, scale_mode mode, std::optional<rect> source
) const -> std::expected<void, error>
{
    assert(handle_);
    assert(destination);
    SDL_Rect const sdl_source = detail::to_sdl(source.value_or(rect{}));
    SDL_Rect const sdl_area = detail::to_sdl(area);
    return detail::check(SDL_BlitSurfaceScaled(
        handle_, source ? &sdl_source : nullptr, destination.handle(),
        &sdl_area, static_cast<SDL_ScaleMode>(mode)
    ));
}

inline auto surface::set_color_key(std::optional<rgba> key) const
    -> std::expected<void, error>
{
    assert(handle_);
    if (!key) return detail::check(SDL_SetSurfaceColorKey(handle_, false, 0));
    return detail::check(SDL_SetSurfaceColorKey(
        handle_, true,
        SDL_MapSurfaceRGBA(handle_, key->r, key->g, key->b, key->a)
    ));
}

inline auto surface::color_key() const noexcept -> std::optional<rgba>
{
    assert(handle_);
    std::uint32_t key = 0;
    if (!SDL_SurfaceHasColorKey(handle_) ||
        !SDL_GetSurfaceColorKey(handle_, &key))
        return std::nullopt;
    SDL_PixelFormatDetails const* const details =
        SDL_GetPixelFormatDetails(handle_->format);
    if (details == nullptr) return std::nullopt;
    rgba result{};
    SDL_GetRGBA(
        key, details, SDL_GetSurfacePalette(handle_), &result.r, &result.g,
        &result.b, &result.a
    );
    return result;
}

inline auto surface::set_color_mod(rgb color) const
    -> std::expected<void, error>
{
    assert(handle_);
    return detail::check(
        SDL_SetSurfaceColorMod(handle_, color.r, color.g, color.b)
    );
}

inline auto surface::color_mod() const -> std::expected<rgb, error>
{
    assert(handle_);
    rgb result{};
    if (!SDL_GetSurfaceColorMod(handle_, &result.r, &result.g, &result.b))
        return std::unexpected(error::current());
    return result;
}

inline auto surface::set_alpha_mod(std::uint8_t alpha) const
    -> std::expected<void, error>
{
    assert(handle_);
    return detail::check(SDL_SetSurfaceAlphaMod(handle_, alpha));
}

inline auto surface::alpha_mod() const -> std::expected<std::uint8_t, error>
{
    assert(handle_);
    std::uint8_t result = 0;
    if (!SDL_GetSurfaceAlphaMod(handle_, &result))
        return std::unexpected(error::current());
    return result;
}

inline auto surface::set_blend_mode(blend_mode mode) const
    -> std::expected<void, error>
{
    assert(handle_);
    return detail::check(
        SDL_SetSurfaceBlendMode(handle_, static_cast<SDL_BlendMode>(mode))
    );
}

inline auto surface::get_blend_mode() const -> std::expected<blend_mode, error>
{
    assert(handle_);
    SDL_BlendMode result = SDL_BLENDMODE_NONE;
    if (!SDL_GetSurfaceBlendMode(handle_, &result))
        return std::unexpected(error::current());
    return static_cast<blend_mode>(result);
}

inline auto surface::set_clip_rect(std::optional<rect> area) const -> bool
{
    assert(handle_);
    SDL_Rect const sdl_area = detail::to_sdl(area.value_or(rect{}));
    return SDL_SetSurfaceClipRect(handle_, area ? &sdl_area : nullptr);
}

inline auto surface::clip_rect() const -> std::expected<rect, error>
{
    assert(handle_);
    SDL_Rect result{};
    if (!SDL_GetSurfaceClipRect(handle_, &result))
        return std::unexpected(error::current());
    return rect{
        .x = result.x, .y = result.y, .width = result.w, .height = result.h
    };
}

inline auto surface::read_pixel(point at) const -> std::expected<rgba, error>
{
    assert(handle_);
    rgba result{};
    if (!SDL_ReadSurfacePixel(
            handle_, at.x, at.y, &result.r, &result.g, &result.b, &result.a
        ))
        return std::unexpected(error::current());
    return result;
}

inline auto surface::write_pixel(point at, rgba color) const
    -> std::expected<void, error>
{
    assert(handle_);
    return detail::check(SDL_WriteSurfacePixel(
        handle_, at.x, at.y, color.r, color.g, color.b, color.a
    ));
}

inline auto surface::lock() const -> std::expected<surface_lock, error>
{
    assert(handle_);
    if (!SDL_LockSurface(handle_)) return std::unexpected(error::current());
    return surface_lock(handle_);
}

inline auto surface::save_bmp(char const* path) const
    -> std::expected<void, error>
{
    assert(handle_);
    return detail::check(SDL_SaveBMP(handle_, path));
}

} // namespace eggs::sdl
