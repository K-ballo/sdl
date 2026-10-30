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
#include <eggs/sdl/error.hpp>
#include <eggs/sdl/event.hpp>
#include <eggs/sdl/geometry.hpp> // IWYU pragma: export
#include <eggs/sdl/pixel_format.hpp>
#include <eggs/sdl/surface.hpp>
#include <eggs/sdl/vertex.hpp> // IWYU pragma: export
#include <eggs/sdl/window.hpp>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <utility>

#include <SDL3/SDL_blendmode.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_video.h>

// The renderer and the textures share this header: a texture cannot exist
// without its renderer, and each names the other.
//
// Requires SDL 3.2.0. Every render call belongs to the thread that created
// the renderer, which for a window is the thread that created the window,
// usually the main thread. The renderer draws in floating point coordinates
// in the current logical space, that is after the logical presentation, the
// scale and the viewport are applied.
namespace eggs::sdl {

// SDL's values. Adaptive is not supported by every driver.
enum class vsync_mode : int
{
    disabled = SDL_RENDERER_VSYNC_DISABLED,
    every_refresh = 1,
    adaptive = SDL_RENDERER_VSYNC_ADAPTIVE,
};

// SDL's values.
enum class logical_presentation_mode : std::int32_t
{
    disabled = SDL_LOGICAL_PRESENTATION_DISABLED,
    stretch = SDL_LOGICAL_PRESENTATION_STRETCH,
    letterbox = SDL_LOGICAL_PRESENTATION_LETTERBOX,
    overscan = SDL_LOGICAL_PRESENTATION_OVERSCAN,
    integer_scale = SDL_LOGICAL_PRESENTATION_INTEGER_SCALE,
};

// The size is zero when the presentation is disabled.
struct logical_presentation_info
{
    extent size;
    logical_presentation_mode mode;
};

// SDL's values. The underscore is only because static is a keyword.
enum class texture_access : std::int32_t
{
    static_ = SDL_TEXTUREACCESS_STATIC,
    streaming = SDL_TEXTUREACCESS_STREAMING,
    target = SDL_TEXTUREACCESS_TARGET,
};

// The number of render drivers that SDL knows about.
[[nodiscard]] inline auto render_driver_count() noexcept -> int
{
    return SDL_GetNumRenderDrivers();
}

// The name of the driver, a simple identifier such as "opengl" or "software"
// that SDL keeps for the lifetime of the program. Empty if the index is out of
// range.
[[nodiscard]] inline auto render_driver(int index) noexcept -> std::string_view
{
    char const* const name = SDL_GetRenderDriver(index);
    return name != nullptr ? std::string_view(name) : std::string_view();
}

class renderer;
class texture;
class texture_lock;

struct renderer_deleter
{
    using pointer = renderer;

    void operator()(renderer owned) const noexcept;
};

// Destroys the renderer, and with it every texture that it created: no texture
// may be used, or destroyed, afterwards. Destroy the textures first.
using unique_renderer = std::unique_ptr<renderer, renderer_deleter>;

struct texture_deleter
{
    using pointer = texture;

    void operator()(texture owned) const noexcept;
};

// Destroys the texture, which must not be the target of a renderer or in use by
// another thread. A texture must be destroyed before its renderer.
using unique_texture = std::unique_ptr<texture, texture_deleter>;

// A handle to a texture, which is pixel data on the renderer's side, in the
// memory of the GPU for a hardware driver. It belongs to the renderer that
// created it: a rendering function called with a texture of another renderer
// fails, and the renderer must outlive its textures, destroying the renderer
// invalidates them. This is documented, not enforced. A texture is used on the
// thread of its renderer.
class texture
{
  public:
    constexpr texture() noexcept = default;

    constexpr texture(std::nullptr_t) noexcept {}

    explicit constexpr texture(SDL_Texture* handle) noexcept
        : handle_(handle)
    {
    }

    // A new texture of the size, of the given pixel format. Render target
    // textures are created with texture_access::target. The pixels are not
    // initialized. Defined with the renderer, below.
    [[nodiscard]] static auto create(
        renderer owner, pixel_format format, texture_access access, extent size
    ) -> std::expected<unique_texture, error>;

    // Copies the pixels: the surface can be released afterwards. The texture
    // is static, and has the pixel format that SDL picks for the surface,
    // which format() tells.
    [[nodiscard]] static auto from_surface(renderer owner, surface source)
        -> std::expected<unique_texture, error>;

    [[nodiscard]] constexpr auto handle() const noexcept -> SDL_Texture*
    {
        return handle_;
    }

    explicit constexpr operator bool() const noexcept
    {
        return handle_ != nullptr;
    }

    constexpr auto operator->() const noexcept -> texture const*
    {
        return this;
    }

    friend constexpr auto operator==(texture, texture) noexcept
        -> bool = default;

    // In pixels. SDL keeps both as public, read-only fields of the texture.
    [[nodiscard]] auto size() const noexcept -> extent;
    [[nodiscard]] auto format() const noexcept -> pixel_format;

    // Replaces the pixels of the area, empty is the whole texture, with the
    // given rows, the pitch bytes apart. This is slow for textures that are
    // updated often, those are streaming and use lock. Fails when the span is
    // smaller than the pixels, which for the FourCC formats is only checked for
    // their first plane: a planar format needs more, and that is the caller's
    // to provide.
    [[nodiscard]] auto update(
        std::optional<rect> area, std::span<std::byte const> pixels, int pitch
    ) const -> std::expected<void, error>;

    // Gives write-only access to the pixels of the area, empty is the whole
    // texture. Streaming textures only. The pixels are not initialized and
    // reading them is undefined, as SDL defines it. The span of the
    // texture_lock is invalid once the lock is gone: unlock by destroying it,
    // before the texture is drawn.
    [[nodiscard]] auto lock(std::optional<rect> area = {}) const
        -> std::expected<texture_lock, error>;

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
    [[nodiscard]] auto set_scale_mode(scale_mode mode) const
        -> std::expected<void, error>;
    [[nodiscard]] auto get_scale_mode() const
        -> std::expected<scale_mode, error>;

  private:
    SDL_Texture* handle_ = nullptr;
};

// Unlocks the texture when destroyed. The texture must outlive it. It gives
// write-only access to the locked area, and the span is invalid once the lock
// is gone.
class texture_lock
{
  public:
    texture_lock(texture_lock const&) = delete;
    auto operator=(texture_lock const&) -> texture_lock& = delete;

    texture_lock(texture_lock&& other) noexcept
        : texture_(std::exchange(other.texture_, nullptr)),
          pixels_(std::exchange(other.pixels_, nullptr)),
          size_(std::exchange(other.size_, 0)),
          pitch_(std::exchange(other.pitch_, 0))
    {
    }

    auto operator=(texture_lock&& other) noexcept -> texture_lock&
    {
        if (this != &other) {
            release();
            texture_ = std::exchange(other.texture_, nullptr);
            pixels_ = std::exchange(other.pixels_, nullptr);
            size_ = std::exchange(other.size_, 0);
            pitch_ = std::exchange(other.pitch_, 0);
        }
        return *this;
    }

    ~texture_lock() { release(); }

    // The rows of the locked area, the pitch apart, the last one only as long
    // as the area is wide. For a FourCC format only the first plane. Write
    // only, as SDL defines it. Empty once the lock was moved from.
    [[nodiscard]] auto pixels() const noexcept -> std::span<std::byte>
    {
        return {pixels_, size_};
    }

    // The number of bytes between rows, 0 once the lock was moved from.
    [[nodiscard]] auto pitch() const noexcept -> int
    {
        return static_cast<int>(pitch_);
    }

  private:
    friend class texture;

    texture_lock(
        SDL_Texture* locked, std::byte* pixels, std::size_t size, int pitch
    ) noexcept
        : texture_(locked),
          pixels_(pixels),
          size_(size),
          pitch_(static_cast<std::size_t>(pitch))
    {
    }

    void release() noexcept
    {
        if (texture_ != nullptr) SDL_UnlockTexture(texture_);
        texture_ = nullptr;
    }

    SDL_Texture* texture_ = nullptr;
    std::byte* pixels_ = nullptr;
    std::size_t size_ = 0;
    std::size_t pitch_ = 0;
};

// A handle to a 2D renderer, either for a window or for a surface. The
// functions that draw take no arguments that belong to a frame: draw, then
// present. After present the contents of the back buffer are undefined, so
// every frame is redrawn in full.
//
// A null from rect means the whole texture and a null to rect means the whole
// render target, both in the functions that draw textures.
class renderer
{
  public:
    constexpr renderer() noexcept = default;

    constexpr renderer(std::nullptr_t) noexcept {}

    explicit constexpr renderer(SDL_Renderer* handle) noexcept
        : handle_(handle)
    {
    }

    // A renderer for the window, which must outlive it. A null driver lets SDL
    // choose, otherwise it is the name of one of render_driver, or a comma
    // separated list of them that SDL tries in order. A window has either a
    // renderer or a window surface (see window::get_surface), not both.
    [[nodiscard]] static auto
    create(window target, char const* driver = nullptr)
        -> std::expected<unique_renderer, error>;

    // A software renderer that draws on the surface, which must outlive it.
    // It has no window: get_window and set_vsync fail. The surface holds the
    // pixels once the drawing is flushed, by flush or by present.
    [[nodiscard]] static auto create_software(surface target)
        -> std::expected<unique_renderer, error>;

    [[nodiscard]] constexpr auto handle() const noexcept -> SDL_Renderer*
    {
        return handle_;
    }

    explicit constexpr operator bool() const noexcept
    {
        return handle_ != nullptr;
    }

    constexpr auto operator->() const noexcept -> renderer const*
    {
        return this;
    }

    friend constexpr auto operator==(renderer, renderer) noexcept
        -> bool = default;

    // SDL-owned, valid until the renderer is destroyed.
    [[nodiscard]] auto name() const -> std::expected<std::string_view, error>;

    // Borrowed from the renderer: it is valid for as long as its owner is.
    // Fails for a software renderer of a surface, that has no window.
    [[nodiscard]] auto get_window() const -> std::expected<window, error>;

    // The size of the output in pixels, the window's pixel size or the
    // surface's size, regardless of the logical presentation.
    [[nodiscard]] auto output_size() const -> std::expected<extent, error>;

    // -- Presentation

    // Can fail on some drivers, and adaptive is not supported everywhere, so
    // the result is to be checked. Fails for a surface renderer.
    [[nodiscard]] auto set_vsync(vsync_mode mode) const
        -> std::expected<void, error>;
    [[nodiscard]] auto get_vsync() const -> std::expected<vsync_mode, error>;

    // -- State

    // The color that primitives, clear and the debug text draw in. The float
    // overload takes the color as it is, for the drivers that can use it, the
    // 8 bit one is the same color. A braced list does not choose between the
    // two overloads: name the type, rgba{...} or fcolor{...}. An rgb converts
    // to an rgba.
    [[nodiscard]] auto set_draw_color(rgba color) const
        -> std::expected<void, error>;
    [[nodiscard]] auto set_draw_color(fcolor color) const
        -> std::expected<void, error>;

    // Rounded to 8 bits per channel, whichever overload set it.
    [[nodiscard]] auto draw_color() const -> std::expected<rgba, error>;

    // How primitives, clear and the debug text are blended. Textures have
    // their own blend mode.
    [[nodiscard]] auto set_draw_blend_mode(blend_mode mode) const
        -> std::expected<void, error>;
    [[nodiscard]] auto get_draw_blend_mode() const
        -> std::expected<blend_mode, error>;

    // The area of the target that is drawn on, in target pixels. Empty restores
    // the whole target. The viewport is relative to the logical presentation.
    [[nodiscard]] auto set_viewport(std::optional<rect> area) const
        -> std::expected<void, error>;
    [[nodiscard]] auto viewport() const -> std::expected<rect, error>;

    // Restricts what is drawn to the area, in target pixels relative to the
    // viewport. Empty disables clipping.
    [[nodiscard]] auto set_clip_rect(std::optional<rect> area) const
        -> std::expected<void, error>;
    [[nodiscard]] auto clip_rect() const -> std::expected<rect, error>;
    [[nodiscard]] auto clip_enabled() const noexcept -> bool;

    // Multiplies the coordinates of everything that is drawn.
    [[nodiscard]] auto set_scale(fpoint scale) const
        -> std::expected<void, error>;
    [[nodiscard]] auto scale() const -> std::expected<fpoint, error>;

    // -- Logical presentation and coordinates

    // Draws in a coordinate system of the given size, that is stretched or
    // letterboxed to the output as the mode says, whatever the size of the
    // window is. The mode disabled ignores the size. Events need
    // convert_event_coordinates to be in the same system.
    [[nodiscard]] auto
    set_logical_presentation(extent size, logical_presentation_mode mode) const
        -> std::expected<void, error>;
    [[nodiscard]] auto logical_presentation() const
        -> std::expected<logical_presentation_info, error>;

    // Convert between window coordinates and the logical ones.
    [[nodiscard]] auto coordinates_from_window(fpoint at) const
        -> std::expected<fpoint, error>;
    [[nodiscard]] auto coordinates_to_window(fpoint at) const
        -> std::expected<fpoint, error>;

    // For mouse and touch events: changes the coordinates stored in the event
    // to the logical ones, in place. The alternatives that are extracted from
    // the event afterwards carry them, the ones extracted before do not. Other
    // events are left as they are.
    [[nodiscard]] auto convert_event_coordinates(event& e) const
        -> std::expected<void, error>;

    // -- Render targets

    // Draws on the texture, which has been created with texture_access::target
    // for this renderer, instead of the window. The viewport, the clip rect,
    // the scale and the logical presentation are kept per target.
    [[nodiscard]] auto set_target(texture target) const
        -> std::expected<void, error>;

    // Back to the window.
    [[nodiscard]] auto reset_target() const -> std::expected<void, error>;

    // Null means the window.
    [[nodiscard]] auto target() const noexcept -> texture;

    // -- Frame

    // Fills the viewport, or the target, with the draw color. The clip rect
    // does not apply.
    [[nodiscard]] auto clear() const -> std::expected<void, error>;

    // Shows the frame. The contents of the back buffer are undefined
    // afterwards: draw every frame in full. Not for a render target texture,
    // reset_target first.
    [[nodiscard]] auto present() const -> std::expected<void, error>;

    // Makes the pending drawing happen now, for code that mixes in the native
    // 3D API. Needed only for that, present flushes.
    [[nodiscard]] auto flush() const -> std::expected<void, error>;

    // -- Primitives, in the draw color. The functions that take several points
    // or rectangles draw nothing, successfully, when there are none, and fail
    // when there are more than an int holds.

    [[nodiscard]] auto draw_point(fpoint at) const
        -> std::expected<void, error>;
    [[nodiscard]] auto draw_points(std::span<fpoint const> points) const
        -> std::expected<void, error>;
    [[nodiscard]] auto draw_line(fpoint from, fpoint to) const
        -> std::expected<void, error>;

    // A connected polyline through the points.
    [[nodiscard]] auto draw_lines(std::span<fpoint const> points) const
        -> std::expected<void, error>;

    [[nodiscard]] auto draw_rect(frect area) const
        -> std::expected<void, error>;
    [[nodiscard]] auto draw_rects(std::span<frect const> areas) const
        -> std::expected<void, error>;
    [[nodiscard]] auto fill_rect(frect area) const
        -> std::expected<void, error>;
    [[nodiscard]] auto fill_rects(std::span<frect const> areas) const
        -> std::expected<void, error>;

    // -- Textures. They belong to this renderer, see texture.

    // Copies the part of the source onto the part of the target, stretching.
    [[nodiscard]] auto draw_texture(
        texture source, std::optional<frect> from = {},
        std::optional<frect> to = {}
    ) const -> std::expected<void, error>;

    // As draw_texture, rotated clockwise by the angle in degrees about the
    // center, which is that of the destination if empty, and flipped.
    [[nodiscard]] auto draw_texture_rotated(
        texture source, std::optional<frect> from, std::optional<frect> to,
        double angle_degrees, std::optional<fpoint> center = {},
        flip_mode flip = flip_mode::none
    ) const -> std::expected<void, error>;

    // Repeats the source over the destination, the source scaled first.
    [[nodiscard]] auto draw_texture_tiled(
        texture source, std::optional<frect> from, float scale,
        std::optional<frect> to
    ) const -> std::expected<void, error>;

    // Stretches the center of the source and keeps the corners, the widths and
    // heights given in source pixels and multiplied by the scale, with a scale
    // of 0 for an unscaled copy.
    [[nodiscard]] auto draw_texture_9grid(
        texture source, std::optional<frect> from, float left, float right,
        float top, float bottom, float scale, std::optional<frect> to
    ) const -> std::expected<void, error>;

    // -- Geometry

    // Triangles from the vertices, three at a time or by the indices. A null
    // texture draws untextured ones, with the colors of the vertices. Fails
    // when an index is out of range, or when the counts do not fit an int.
    // Nothing is drawn, successfully, when there are no vertices.
    [[nodiscard]] auto draw_geometry(
        texture source, std::span<vertex const> vertices,
        std::span<int const> indices = {}
    ) const -> std::expected<void, error>;

    // -- Debug text

    // Draws the text with SDL's built-in 8 by 8 pixel font, ASCII only, in
    // the draw color and at the scale of the renderer, with the top left at the
    // point. For tools and demos.
    [[nodiscard]] auto draw_debug_text(fpoint at, char const* text) const
        -> std::expected<void, error>;

    // -- Readback

    // The pixels of the area of the viewport, empty is all of it, in a new
    // surface that the caller owns. With a logical presentation the viewport
    // is the part of the target that it draws on, so the bars of a letterbox
    // are not read. This is slow, and stalls the pipeline.
    // Read before present, the back buffer is undefined after it.
    [[nodiscard]] auto read_pixels(std::optional<rect> area = {}) const
        -> std::expected<unique_surface, error>;

  private:
    SDL_Renderer* handle_ = nullptr;
};

namespace detail {

[[nodiscard]] constexpr auto to_sdl(frect value) noexcept -> SDL_FRect
{
    return {.x = value.x, .y = value.y, .w = value.width, .h = value.height};
}

[[nodiscard]] constexpr auto to_sdl(fpoint value) noexcept -> SDL_FPoint
{
    return {.x = value.x, .y = value.y};
}

// Whether the number of elements is one that SDL's int counts can hold.
[[nodiscard]] constexpr auto fits_int(std::size_t count) noexcept -> bool
{
    return count <= static_cast<std::size_t>(std::numeric_limits<int>::max());
}

[[nodiscard]] inline auto too_many() -> std::unexpected<error>
{
    (void)SDL_SetError("There are too many elements");
    return std::unexpected(error::current());
}

} // namespace detail

inline void renderer_deleter::operator()(renderer owned) const noexcept
{
    SDL_DestroyRenderer(owned.handle());
}

inline void texture_deleter::operator()(texture owned) const noexcept
{
    SDL_DestroyTexture(owned.handle());
}

// -- texture

inline auto texture::create(
    renderer owner, pixel_format format, texture_access access, extent size
) -> std::expected<unique_texture, error>
{
    assert(owner);
    SDL_Texture* const created = SDL_CreateTexture(
        owner.handle(), static_cast<SDL_PixelFormat>(format),
        static_cast<SDL_TextureAccess>(access), size.width, size.height
    );
    if (created == nullptr) return std::unexpected(error::current());
    return unique_texture(texture(created));
}

inline auto texture::from_surface(renderer owner, surface source)
    -> std::expected<unique_texture, error>
{
    assert(owner);
    assert(source);
    SDL_Texture* const created =
        SDL_CreateTextureFromSurface(owner.handle(), source.handle());
    if (created == nullptr) return std::unexpected(error::current());
    return unique_texture(texture(created));
}

inline auto texture::size() const noexcept -> extent
{
    assert(handle_);
    return {.width = handle_->w, .height = handle_->h};
}

inline auto texture::format() const noexcept -> pixel_format
{
    assert(handle_);
    return static_cast<pixel_format>(handle_->format);
}

inline auto texture::update(
    std::optional<rect> area, std::span<std::byte const> pixels, int pitch
) const -> std::expected<void, error>
{
    assert(handle_);
    extent const covered =
        area ? extent{.width = area->width, .height = area->height} : size();
    // SDL validates the rest, this is only what it cannot see.
    if (covered.width > 0 && covered.height > 0 && pitch > 0 &&
        pixels.size() < detail::minimum_size(covered, format(), pitch)) {
        (void)SDL_SetError("The pixel buffer is smaller than the area");
        return std::unexpected(error::current());
    }
    SDL_Rect const sdl_area = detail::to_sdl(area.value_or(rect{}));
    return detail::check(SDL_UpdateTexture(
        handle_, area ? &sdl_area : nullptr, pixels.data(), pitch
    ));
}

inline auto texture::lock(std::optional<rect> area) const
    -> std::expected<texture_lock, error>
{
    assert(handle_);
    SDL_Rect const sdl_area = detail::to_sdl(area.value_or(rect{}));
    void* pixels = nullptr;
    int pitch = 0;
    if (!SDL_LockTexture(handle_, area ? &sdl_area : nullptr, &pixels, &pitch))
        return std::unexpected(error::current());

    extent const locked =
        area ? extent{.width = area->width, .height = area->height} : size();
    std::size_t const bytes =
        locked.width > 0 && locked.height > 0 && pitch > 0
            ? detail::minimum_size(locked, format(), pitch)
            : 0;
    return texture_lock(handle_, static_cast<std::byte*>(pixels), bytes, pitch);
}

inline auto texture::set_color_mod(rgb color) const
    -> std::expected<void, error>
{
    assert(handle_);
    return detail::check(
        SDL_SetTextureColorMod(handle_, color.r, color.g, color.b)
    );
}

inline auto texture::color_mod() const -> std::expected<rgb, error>
{
    assert(handle_);
    rgb result{};
    if (!SDL_GetTextureColorMod(handle_, &result.r, &result.g, &result.b))
        return std::unexpected(error::current());
    return result;
}

inline auto texture::set_alpha_mod(std::uint8_t alpha) const
    -> std::expected<void, error>
{
    assert(handle_);
    return detail::check(SDL_SetTextureAlphaMod(handle_, alpha));
}

inline auto texture::alpha_mod() const -> std::expected<std::uint8_t, error>
{
    assert(handle_);
    std::uint8_t result = 0;
    if (!SDL_GetTextureAlphaMod(handle_, &result))
        return std::unexpected(error::current());
    return result;
}

inline auto texture::set_blend_mode(blend_mode mode) const
    -> std::expected<void, error>
{
    assert(handle_);
    return detail::check(
        SDL_SetTextureBlendMode(handle_, static_cast<SDL_BlendMode>(mode))
    );
}

inline auto texture::get_blend_mode() const -> std::expected<blend_mode, error>
{
    assert(handle_);
    SDL_BlendMode result = SDL_BLENDMODE_NONE;
    if (!SDL_GetTextureBlendMode(handle_, &result))
        return std::unexpected(error::current());
    return static_cast<blend_mode>(result);
}

inline auto texture::set_scale_mode(scale_mode mode) const
    -> std::expected<void, error>
{
    assert(handle_);
    return detail::check(
        SDL_SetTextureScaleMode(handle_, static_cast<SDL_ScaleMode>(mode))
    );
}

inline auto texture::get_scale_mode() const -> std::expected<scale_mode, error>
{
    assert(handle_);
    SDL_ScaleMode result = SDL_SCALEMODE_NEAREST;
    if (!SDL_GetTextureScaleMode(handle_, &result))
        return std::unexpected(error::current());
    return static_cast<scale_mode>(result);
}

// -- renderer

inline auto renderer::create(window target, char const* driver)
    -> std::expected<unique_renderer, error>
{
    assert(target);
    SDL_Renderer* const created = SDL_CreateRenderer(target.handle(), driver);
    if (created == nullptr) return std::unexpected(error::current());
    return unique_renderer(renderer(created));
}

inline auto renderer::create_software(surface target)
    -> std::expected<unique_renderer, error>
{
    assert(target);
    SDL_Renderer* const created = SDL_CreateSoftwareRenderer(target.handle());
    if (created == nullptr) return std::unexpected(error::current());
    return unique_renderer(renderer(created));
}

inline auto renderer::name() const -> std::expected<std::string_view, error>
{
    assert(handle_);
    char const* const result = SDL_GetRendererName(handle_);
    if (result == nullptr) return std::unexpected(error::current());
    return std::string_view(result);
}

inline auto renderer::get_window() const -> std::expected<window, error>
{
    assert(handle_);
    SDL_Window* const result = SDL_GetRenderWindow(handle_);
    if (result == nullptr) {
        // SDL does not set an error for a renderer without a window.
        (void)SDL_SetError("The renderer has no window");
        return std::unexpected(error::current());
    }
    return window(result);
}

inline auto renderer::output_size() const -> std::expected<extent, error>
{
    assert(handle_);
    extent result{};
    if (!SDL_GetCurrentRenderOutputSize(handle_, &result.width, &result.height))
        return std::unexpected(error::current());
    return result;
}

inline auto renderer::set_vsync(vsync_mode mode) const
    -> std::expected<void, error>
{
    assert(handle_);
    return detail::check(SDL_SetRenderVSync(handle_, static_cast<int>(mode)));
}

inline auto renderer::get_vsync() const -> std::expected<vsync_mode, error>
{
    assert(handle_);
    int result = 0;
    if (!SDL_GetRenderVSync(handle_, &result))
        return std::unexpected(error::current());
    return static_cast<vsync_mode>(result);
}

inline auto renderer::set_draw_color(rgba color) const
    -> std::expected<void, error>
{
    assert(handle_);
    return detail::check(
        SDL_SetRenderDrawColor(handle_, color.r, color.g, color.b, color.a)
    );
}

inline auto renderer::set_draw_color(fcolor color) const
    -> std::expected<void, error>
{
    assert(handle_);
    return detail::check(
        SDL_SetRenderDrawColorFloat(handle_, color.r, color.g, color.b, color.a)
    );
}

inline auto renderer::draw_color() const -> std::expected<rgba, error>
{
    assert(handle_);
    rgba result{};
    if (!SDL_GetRenderDrawColor(
            handle_, &result.r, &result.g, &result.b, &result.a
        ))
        return std::unexpected(error::current());
    return result;
}

inline auto renderer::set_draw_blend_mode(blend_mode mode) const
    -> std::expected<void, error>
{
    assert(handle_);
    return detail::check(
        SDL_SetRenderDrawBlendMode(handle_, static_cast<SDL_BlendMode>(mode))
    );
}

inline auto renderer::get_draw_blend_mode() const
    -> std::expected<blend_mode, error>
{
    assert(handle_);
    SDL_BlendMode result = SDL_BLENDMODE_NONE;
    if (!SDL_GetRenderDrawBlendMode(handle_, &result))
        return std::unexpected(error::current());
    return static_cast<blend_mode>(result);
}

inline auto renderer::set_viewport(std::optional<rect> area) const
    -> std::expected<void, error>
{
    assert(handle_);
    SDL_Rect const sdl_area = detail::to_sdl(area.value_or(rect{}));
    return detail::check(
        SDL_SetRenderViewport(handle_, area ? &sdl_area : nullptr)
    );
}

inline auto renderer::viewport() const -> std::expected<rect, error>
{
    assert(handle_);
    SDL_Rect result{};
    if (!SDL_GetRenderViewport(handle_, &result))
        return std::unexpected(error::current());
    return rect{
        .x = result.x, .y = result.y, .width = result.w, .height = result.h
    };
}

inline auto renderer::set_clip_rect(std::optional<rect> area) const
    -> std::expected<void, error>
{
    assert(handle_);
    SDL_Rect const sdl_area = detail::to_sdl(area.value_or(rect{}));
    return detail::check(
        SDL_SetRenderClipRect(handle_, area ? &sdl_area : nullptr)
    );
}

inline auto renderer::clip_rect() const -> std::expected<rect, error>
{
    assert(handle_);
    SDL_Rect result{};
    if (!SDL_GetRenderClipRect(handle_, &result))
        return std::unexpected(error::current());
    return rect{
        .x = result.x, .y = result.y, .width = result.w, .height = result.h
    };
}

inline auto renderer::clip_enabled() const noexcept -> bool
{
    assert(handle_);
    return SDL_RenderClipEnabled(handle_);
}

inline auto renderer::set_scale(fpoint scale) const
    -> std::expected<void, error>
{
    assert(handle_);
    return detail::check(SDL_SetRenderScale(handle_, scale.x, scale.y));
}

inline auto renderer::scale() const -> std::expected<fpoint, error>
{
    assert(handle_);
    fpoint result{};
    if (!SDL_GetRenderScale(handle_, &result.x, &result.y))
        return std::unexpected(error::current());
    return result;
}

inline auto renderer::set_logical_presentation(
    extent size, logical_presentation_mode mode
) const -> std::expected<void, error>
{
    assert(handle_);
    return detail::check(SDL_SetRenderLogicalPresentation(
        handle_, size.width, size.height,
        static_cast<SDL_RendererLogicalPresentation>(mode)
    ));
}

inline auto renderer::logical_presentation() const
    -> std::expected<logical_presentation_info, error>
{
    assert(handle_);
    logical_presentation_info result{};
    SDL_RendererLogicalPresentation mode = SDL_LOGICAL_PRESENTATION_DISABLED;
    if (!SDL_GetRenderLogicalPresentation(
            handle_, &result.size.width, &result.size.height, &mode
        ))
        return std::unexpected(error::current());
    result.mode = static_cast<logical_presentation_mode>(mode);
    return result;
}

inline auto renderer::coordinates_from_window(fpoint at) const
    -> std::expected<fpoint, error>
{
    assert(handle_);
    fpoint result{};
    if (!SDL_RenderCoordinatesFromWindow(
            handle_, at.x, at.y, &result.x, &result.y
        ))
        return std::unexpected(error::current());
    return result;
}

inline auto renderer::coordinates_to_window(fpoint at) const
    -> std::expected<fpoint, error>
{
    assert(handle_);
    fpoint result{};
    if (!SDL_RenderCoordinatesToWindow(
            handle_, at.x, at.y, &result.x, &result.y
        ))
        return std::unexpected(error::current());
    return result;
}

inline auto renderer::convert_event_coordinates(event& e) const
    -> std::expected<void, error>
{
    assert(handle_);
    return detail::check(
        SDL_ConvertEventToRenderCoordinates(handle_, &e.sdl())
    );
}

inline auto renderer::set_target(texture target) const
    -> std::expected<void, error>
{
    assert(handle_);
    assert(target);
    return detail::check(SDL_SetRenderTarget(handle_, target.handle()));
}

inline auto renderer::reset_target() const -> std::expected<void, error>
{
    assert(handle_);
    return detail::check(SDL_SetRenderTarget(handle_, nullptr));
}

inline auto renderer::target() const noexcept -> texture
{
    assert(handle_);
    return texture(SDL_GetRenderTarget(handle_));
}

inline auto renderer::clear() const -> std::expected<void, error>
{
    assert(handle_);
    return detail::check(SDL_RenderClear(handle_));
}

inline auto renderer::present() const -> std::expected<void, error>
{
    assert(handle_);
    return detail::check(SDL_RenderPresent(handle_));
}

inline auto renderer::flush() const -> std::expected<void, error>
{
    assert(handle_);
    return detail::check(SDL_FlushRenderer(handle_));
}

inline auto renderer::draw_point(fpoint at) const -> std::expected<void, error>
{
    assert(handle_);
    return detail::check(SDL_RenderPoint(handle_, at.x, at.y));
}

inline auto renderer::draw_points(std::span<fpoint const> points) const
    -> std::expected<void, error>
{
    assert(handle_);
    if (points.empty()) return {};
    if (!detail::fits_int(points.size())) return detail::too_many();
    // The layouts are identical, see geometry.hpp.
    return detail::check(SDL_RenderPoints(
        handle_, reinterpret_cast<SDL_FPoint const*>(points.data()),
        static_cast<int>(points.size())
    ));
}

inline auto renderer::draw_line(fpoint from, fpoint to) const
    -> std::expected<void, error>
{
    assert(handle_);
    return detail::check(SDL_RenderLine(handle_, from.x, from.y, to.x, to.y));
}

inline auto renderer::draw_lines(std::span<fpoint const> points) const
    -> std::expected<void, error>
{
    assert(handle_);
    if (points.empty()) return {};
    if (!detail::fits_int(points.size())) return detail::too_many();
    return detail::check(SDL_RenderLines(
        handle_, reinterpret_cast<SDL_FPoint const*>(points.data()),
        static_cast<int>(points.size())
    ));
}

inline auto renderer::draw_rect(frect area) const -> std::expected<void, error>
{
    assert(handle_);
    SDL_FRect const sdl_area = detail::to_sdl(area);
    return detail::check(SDL_RenderRect(handle_, &sdl_area));
}

inline auto renderer::draw_rects(std::span<frect const> areas) const
    -> std::expected<void, error>
{
    assert(handle_);
    if (areas.empty()) return {};
    if (!detail::fits_int(areas.size())) return detail::too_many();
    return detail::check(SDL_RenderRects(
        handle_, reinterpret_cast<SDL_FRect const*>(areas.data()),
        static_cast<int>(areas.size())
    ));
}

inline auto renderer::fill_rect(frect area) const -> std::expected<void, error>
{
    assert(handle_);
    SDL_FRect const sdl_area = detail::to_sdl(area);
    return detail::check(SDL_RenderFillRect(handle_, &sdl_area));
}

inline auto renderer::fill_rects(std::span<frect const> areas) const
    -> std::expected<void, error>
{
    assert(handle_);
    if (areas.empty()) return {};
    if (!detail::fits_int(areas.size())) return detail::too_many();
    return detail::check(SDL_RenderFillRects(
        handle_, reinterpret_cast<SDL_FRect const*>(areas.data()),
        static_cast<int>(areas.size())
    ));
}

inline auto renderer::draw_texture(
    texture source, std::optional<frect> from, std::optional<frect> to
) const -> std::expected<void, error>
{
    assert(handle_);
    assert(source);
    SDL_FRect const sdl_from = detail::to_sdl(from.value_or(frect{}));
    SDL_FRect const sdl_to = detail::to_sdl(to.value_or(frect{}));
    return detail::check(SDL_RenderTexture(
        handle_, source.handle(), from ? &sdl_from : nullptr,
        to ? &sdl_to : nullptr
    ));
}

inline auto renderer::draw_texture_rotated(
    texture source, std::optional<frect> from, std::optional<frect> to,
    double angle_degrees, std::optional<fpoint> center, flip_mode flip
) const -> std::expected<void, error>
{
    assert(handle_);
    assert(source);
    SDL_FRect const sdl_from = detail::to_sdl(from.value_or(frect{}));
    SDL_FRect const sdl_to = detail::to_sdl(to.value_or(frect{}));
    SDL_FPoint const sdl_center = detail::to_sdl(center.value_or(fpoint{}));
    return detail::check(SDL_RenderTextureRotated(
        handle_, source.handle(), from ? &sdl_from : nullptr,
        to ? &sdl_to : nullptr, angle_degrees, center ? &sdl_center : nullptr,
        static_cast<SDL_FlipMode>(flip)
    ));
}

inline auto renderer::draw_texture_tiled(
    texture source, std::optional<frect> from, float scale,
    std::optional<frect> to
) const -> std::expected<void, error>
{
    assert(handle_);
    assert(source);
    SDL_FRect const sdl_from = detail::to_sdl(from.value_or(frect{}));
    SDL_FRect const sdl_to = detail::to_sdl(to.value_or(frect{}));
    return detail::check(SDL_RenderTextureTiled(
        handle_, source.handle(), from ? &sdl_from : nullptr, scale,
        to ? &sdl_to : nullptr
    ));
}

inline auto renderer::draw_texture_9grid(
    texture source, std::optional<frect> from, float left, float right,
    float top, float bottom, float scale, std::optional<frect> to
) const -> std::expected<void, error>
{
    assert(handle_);
    assert(source);
    SDL_FRect const sdl_from = detail::to_sdl(from.value_or(frect{}));
    SDL_FRect const sdl_to = detail::to_sdl(to.value_or(frect{}));
    return detail::check(SDL_RenderTexture9Grid(
        handle_, source.handle(), from ? &sdl_from : nullptr, left, right, top,
        bottom, scale, to ? &sdl_to : nullptr
    ));
}

inline auto renderer::draw_geometry(
    texture source, std::span<vertex const> vertices,
    std::span<int const> indices
) const -> std::expected<void, error>
{
    assert(handle_);
    if (vertices.empty()) return {};
    if (!detail::fits_int(vertices.size()) || !detail::fits_int(indices.size()))
        return detail::too_many();
    // The layout of vertex is that of SDL_Vertex, see vertex.hpp.
    return detail::check(SDL_RenderGeometry(
        handle_, source.handle(),
        reinterpret_cast<SDL_Vertex const*>(vertices.data()),
        static_cast<int>(vertices.size()),
        indices.empty() ? nullptr : indices.data(),
        static_cast<int>(indices.size())
    ));
}

inline auto renderer::draw_debug_text(fpoint at, char const* text) const
    -> std::expected<void, error>
{
    assert(handle_);
    return detail::check(SDL_RenderDebugText(handle_, at.x, at.y, text));
}

inline auto renderer::read_pixels(std::optional<rect> area) const
    -> std::expected<unique_surface, error>
{
    assert(handle_);
    SDL_Rect const sdl_area = detail::to_sdl(area.value_or(rect{}));
    return detail::owned(
        SDL_RenderReadPixels(handle_, area ? &sdl_area : nullptr)
    );
}

} // namespace eggs::sdl
