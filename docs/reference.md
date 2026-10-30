<!--
Eggs.SDL

Copyright (c) 2026 Agustin Berge

Distributed under the Boost Software License, Version 1.0. (See
accompanying file LICENSE.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
-->

# Reference

## Headers

| Header | Provides |
|--------|----------|
| `<eggs/sdl.hpp>` | Everything below |
| `<eggs/sdl/error.hpp>` | `error` |
| `<eggs/sdl/init.hpp>` | `init_flags`, `subsystems`, `unique_subsystems`, `init`, `quit`, `was_init`, `set_app_metadata`, `set_app_metadata_property` |
| `<eggs/sdl/window.hpp>` | `window_flags`, `window_id`, `window`, `unique_window`, `extent`, `point`, `rect` |
| `<eggs/sdl/geometry.hpp>` | `fpoint`, `fextent`, `frect`, and `extent`, `point`, `rect` |
| `<eggs/sdl/color.hpp>` | `rgb`, `rgba`, `fcolor` |
| `<eggs/sdl/pixel_format.hpp>` | `pixel_format`, `name`, `bits_per_pixel`, `bytes_per_pixel`, `has_alpha` |
| `<eggs/sdl/blend_mode.hpp>` | `blend_mode` |
| `<eggs/sdl/surface.hpp>` | `scale_mode`, `flip_mode`, `surface`, `unique_surface`, `surface_lock` |
| `<eggs/sdl/renderer.hpp>` | `vsync_mode`, `logical_presentation_mode`, `logical_presentation_info`, `texture_access`, `render_driver_count`, `render_driver`, `renderer`, `unique_renderer`, `texture`, `unique_texture`, `texture_lock` |
| `<eggs/sdl/vertex.hpp>` | `vertex` |
| `<eggs/sdl/time.hpp>` | `clock`, `performance_counter`, `performance_frequency`, `delay`, `delay_precise` |
| `<eggs/sdl/keyboard.hpp>` | `scancode`, `keycode`, `key_modifiers`, `name` |
| `<eggs/sdl/event.hpp>` | `event_type`, the event alternatives, `event`, the queue functions, `event_watch`, `event_filter` |

Headers under `eggs/sdl/detail/` are implementation details, except for
the `EGGS_SDL_EVENT_TYPES` and `EGGS_SDL_PIXEL_FORMATS` tables.

The SDL version required is 3.2.0, and the latest release the library is
tested against is 3.4.18. What a newer release adds is available when
compiled against it, and is absent when compiled against an older one:
`pinch_begin_event`, `pinch_update_event`, `pinch_end_event`,
`screen_keyboard_shown_event`, `screen_keyboard_hidden_event`,
`display_usable_bounds_changed_event`, `scancode::front`,
`window_flags::fill_document`, `progress_state`, the window members that
use it, `pixel_format::mjpg`, `scale_mode::pixelart` and
`flip_mode::horizontal_and_vertical` need SDL 3.4.0. The SDL library loaded at run time must be at least
as new as the one compiled against. Every function
that returns `std::expected<T, error>` is `[[nodiscard]]`.

---

## `eggs::sdl::error`

```cpp
class error
{
  public:
    static auto current() -> error;
    auto message() const noexcept -> std::string_view;
};
```

`current()` captures `SDL_GetError()` for the calling thread. Call it
right after the failing SDL call. `message()` is empty when SDL failed
without setting an error (see `push`).

---

## Init

```cpp
enum class init_flags : std::uint32_t
{
    none, audio, video, joystick, haptic, gamepad, events, sensor, camera
};
```

SDL's values, with the bitwise operators. Video implies events and
gamepad implies joystick.

```cpp
class subsystems;                            // the flags an owner initialized
struct subsystems_deleter;                   // SDL_QuitSubSystem, never SDL_Quit
using unique_subsystems = std::unique_ptr<subsystems, subsystems_deleter>;

auto init(init_flags flags) -> std::expected<unique_subsystems, error>;
void quit() noexcept;
auto was_init(init_flags flags = init_flags::none) noexcept -> init_flags;
auto set_app_metadata(char const* name, char const* version, char const* identifier)
    -> std::expected<void, error>;
auto set_app_metadata_property(char const* name, char const* value)
    -> std::expected<void, error>;
```

`init` requires a non-empty `flags`. Destroying the owner quits only the
subsystems it initialized. Video must be initialized, and used, from the
main thread. `quit` is `SDL_Quit`, owners are not aware of it.
`was_init` returns the initialized subset of `flags`, or all of the
initialized subsystems when `flags` is empty.

---

## Geometry

```cpp
struct extent { int width; int height; };
struct point  { int x; int y; };
struct rect   { int x; int y; int width; int height; };
struct rgb    { std::uint8_t r; std::uint8_t g; std::uint8_t b; };
```

The `f` variants are for the float geometry of SDL:

```cpp
struct fpoint  { float x; float y; };
struct fextent { float width; float height; };
struct frect   { float x; float y; float width; float height; };
```

`fpoint` and `frect` have the layout of `SDL_FPoint` and `SDL_FRect`, which
is checked at compile time, so arrays of them can be handed to SDL as they
are.

---

## Color

```cpp
struct rgb    { std::uint8_t r; std::uint8_t g; std::uint8_t b; operator rgba() const noexcept; };
struct rgba   { std::uint8_t r; std::uint8_t g; std::uint8_t b; std::uint8_t a = 255; };
struct fcolor { float r; float g; float b; float a = 1.0F; };
```

Plain aggregates. `rgba` and `fcolor` have the layout of `SDL_Color` and
`SDL_FColor`. An `rgb` converts to an opaque `rgba` implicitly, through a
conversion function on `rgb`, so that `rgba` stays an aggregate: it can be
initialized with braces and with designated initializers, and `a` may be
left out.

---

## Pixel format

```cpp
enum class pixel_format : std::uint32_t { /* every SDL3 pixel format */ };
auto name(pixel_format format) noexcept -> std::string_view;
constexpr auto bits_per_pixel(pixel_format format) noexcept -> int;
constexpr auto bytes_per_pixel(pixel_format format) noexcept -> int;
constexpr auto has_alpha(pixel_format format) noexcept -> bool;

enum class blend_mode : std::uint32_t { none, blend, blend_premultiplied, add, add_premultiplied, mod, mul };
```

The enumerators are SDL's formats in snake case without the prefix, with
SDL's values, including the endian-dependent aliases such as `rgba32`
(`rgba8888` or `abgr8888`, so that its bytes are in the order r, g, b, a in
memory), and `unknown`. `mjpg` needs SDL 3.4.0. The table they are generated
from is `EGGS_SDL_PIXEL_FORMATS(X)`, in the same way as the events.

`name` is SDL-owned and static, `"SDL_PIXELFORMAT_UNKNOWN"` for a value that
is not a pixel format. The FourCC formats, the YUV ones, have 0 bits per
pixel and, as SDL defines it, a best effort 1 or 2 bytes per pixel.

`blend_mode` can hold other values than its enumerators, as SDL can compose
custom blend modes, and a value read back from SDL may be one.

---

## Window

```cpp
enum class window_flags : std::uint64_t { /* every SDL3 window flag */ };
enum class window_id : std::uint32_t {};   // 0 is invalid

class window
{
  public:
    constexpr window() noexcept = default;
    constexpr window(std::nullptr_t) noexcept;
    explicit constexpr window(SDL_Window* handle) noexcept;

    static auto create(char const* title, int width, int height,
                       window_flags flags = window_flags::none)
        -> std::expected<unique_window, error>;
    static auto from_id(window_id id) -> std::expected<window, error>;

    constexpr auto handle() const noexcept -> SDL_Window*;
    constexpr explicit operator bool() const noexcept;

    auto id() const -> std::expected<window_id, error>;
    auto title() const noexcept -> std::string_view;
    auto set_title(char const* title) const -> std::expected<void, error>;
    auto size() const -> std::expected<extent, error>;
    auto set_size(extent size) const -> std::expected<void, error>;
    auto pixel_size() const -> std::expected<extent, error>;
    auto show() const -> std::expected<void, error>;
    auto hide() const -> std::expected<void, error>;
    auto raise() const -> std::expected<void, error>;

    auto get_surface() const -> std::expected<surface, error>;
    auto has_surface() const noexcept -> bool;
    auto update_surface() const -> std::expected<void, error>;
    auto update_surface(std::span<rect const> rects) const -> std::expected<void, error>;
    auto destroy_surface() const -> std::expected<void, error>;

    // SDL 3.4.0 and later
    auto set_fill_document(bool fill) const -> std::expected<void, error>;
    auto set_progress_state(progress_state state) const -> std::expected<void, error>;
    auto get_progress_state() const -> std::expected<progress_state, error>;
    auto set_progress_value(float value) const -> std::expected<void, error>;
    auto progress_value() const -> std::expected<float, error>;
};

struct window_deleter;
using unique_window = std::unique_ptr<window, window_deleter>;
```

`window` is a trivially copyable, pointer-sized handle that owns nothing.
It compares equal to another handle with the same pointer, and to
`nullptr`. Window functions must be called from the main thread. The
member functions assert that the handle is not null in debug builds.

- **`from_id`** returns a borrowed window, valid as long as its owner.
- **`title`** is SDL-owned, valid until the next `set_title` or until the
  window is destroyed. It is empty on error.
- **`get_surface`** returns the surface that belongs to the window. It is
  invalidated by `destroy_surface`, by destroying the window, and by a
  resize or a change of the pixel size of the window. It fails for
  windows created with a 3D API flag.
- **`set_fill_document`**, **`set_progress_state`**, **`get_progress_state`**,
  **`set_progress_value`** and **`progress_value`** need SDL 3.4.0.
  `progress_state` is `none`, `indeterminate`, `normal`, `paused` or
  `error`, with SDL's values. SDL reports a failed query with
  `SDL_PROGRESS_STATE_INVALID` or a value of -1, which are errors here.
  SDL clamps a progress value outside 0 to 1. `set_fill_document` only
  acts on Emscripten.

---

## Surface

```cpp
enum class scale_mode : std::int32_t { nearest, linear /* , pixelart */ };
enum class flip_mode  : std::int32_t { none, horizontal, vertical /* , horizontal_and_vertical */ };

using unique_surface = std::unique_ptr<surface, surface_deleter>;

class surface
{
  public:
    constexpr surface() noexcept = default;
    constexpr surface(std::nullptr_t) noexcept;
    explicit constexpr surface(SDL_Surface* handle) noexcept;

    static auto create(extent size, pixel_format format) -> std::expected<unique_surface, error>;
    static auto from_pixels(extent size, pixel_format format, std::span<std::byte> pixels, int pitch)
        -> std::expected<unique_surface, error>;
    static auto load_bmp(char const* path) -> std::expected<unique_surface, error>;

    constexpr auto handle() const noexcept -> SDL_Surface*;
    constexpr explicit operator bool() const noexcept;

    auto size() const noexcept -> extent;
    auto format() const noexcept -> pixel_format;
    auto pitch() const noexcept -> int;

    auto duplicate() const -> std::expected<unique_surface, error>;
    auto convert(pixel_format format) const -> std::expected<unique_surface, error>;
    auto scale(extent size, scale_mode mode) const -> std::expected<unique_surface, error>;

    auto flip(flip_mode mode) const -> std::expected<void, error>;
    auto premultiply_alpha(bool linear = false) const -> std::expected<void, error>;

    auto clear(fcolor color) const -> std::expected<void, error>;
    auto fill(rgba color) const -> std::expected<void, error>;
    auto fill(rect area, rgba color) const -> std::expected<void, error>;
    auto fill(std::span<rect const> areas, rgba color) const -> std::expected<void, error>;
    auto blit(surface destination, point at = {}, std::optional<rect> source = {}) const
        -> std::expected<void, error>;
    auto blit_scaled(surface destination, rect area, scale_mode mode, std::optional<rect> source = {}) const
        -> std::expected<void, error>;

    auto set_color_key(std::optional<rgba> key) const -> std::expected<void, error>;
    auto color_key() const noexcept -> std::optional<rgba>;
    auto set_color_mod(rgb color) const -> std::expected<void, error>;
    auto color_mod() const -> std::expected<rgb, error>;
    auto set_alpha_mod(std::uint8_t alpha) const -> std::expected<void, error>;
    auto alpha_mod() const -> std::expected<std::uint8_t, error>;
    auto set_blend_mode(blend_mode mode) const -> std::expected<void, error>;
    auto get_blend_mode() const -> std::expected<blend_mode, error>;
    auto set_clip_rect(std::optional<rect> area) const -> bool;
    auto clip_rect() const -> std::expected<rect, error>;

    auto read_pixel(point at) const -> std::expected<rgba, error>;
    auto write_pixel(point at, rgba color) const -> std::expected<void, error>;

    auto lock() const -> std::expected<surface_lock, error>;
    auto save_bmp(char const* path) const -> std::expected<void, error>;
};

class surface_lock   // move-only, unlocks when destroyed
{
  public:
    auto pixels() const noexcept -> std::span<std::byte>;
    auto pitch() const noexcept -> int;
};
```

A `surface` is either one the caller owns, through a `unique_surface`, or
a borrowed one, such as the window surface (see `window::get_surface` for
its validity). Every member works on both. Do not put a borrowed surface in
a `unique_surface`. `scale_mode::pixelart` and
`flip_mode::horizontal_and_vertical` need SDL 3.4.0.

- **`create`** zero fills the surface. A negative size fails, a size of
  zero does not.
- **`from_pixels`** does not copy: the memory must outlive the surface, and
  must not be accessed except through a `surface_lock` while the surface
  uses it. It fails when the span is smaller than the pixels, which for the
  FourCC formats is only checked for their first plane.
- **`duplicate`, `convert` and `scale`** return new surfaces, and never
  modify the source.
- **`flip` and `premultiply_alpha`** modify the surface in place.
- **`clear`** sets every pixel. **`fill`** sets the pixels of the area, or
  of the whole surface, to the color as the format represents it, alpha
  included. Only `fill` is clipped by the clip rect.
- **`blit`** copies the surface onto the destination, with the blend mode,
  color and alpha modulation and color key of the source, and the clip rect
  of the destination. The coordinates are in destination pixels. Only the
  position of the destination is used and the size is that of the source,
  or of the source rect. **`blit_scaled`** stretches the source to the area.
  Surfaces of different pixel formats can be blitted, SDL converts as it
  goes. Converting a surface once to the format of the destination is faster
  when it is blitted repeatedly.
- **`color_key`** returns the key as the format of the surface holds it,
  which is the color that was set only as far as the format can represent
  it, and is empty when there is no key.
- **`set_clip_rect`** returns whether the area intersects the surface. False
  is not a failure: SDL then clips every blit completely, and does not
  report an error, so it is a `bool` and not an `expected`.
- **`read_pixel` and `write_pixel`** are slow, for tools and tests.
- **`lock`**: pixel memory may be accessed only while the `surface_lock` is
  alive. Its span, of the pitch times the height bytes, is invalid once the
  lock or the surface is gone.
- **`load_bmp` and `save_bmp`** are the only file formats.

Surfaces may be created and loaded on any thread. A surface is used by one
thread at a time, and `blit` and `blit_scaled` need exclusive use of both
surfaces. Different surfaces may be used on different threads.

---

## Renderer

The 2D render API: the renderer, textures, drawing, render targets, vsync
and logical presentation. It needs SDL 3.2.0, no more. Textures are in the
header of the renderer, since each names the other.

```cpp
enum class vsync_mode : int { disabled = 0, every_refresh = 1, adaptive = -1 };
enum class logical_presentation_mode { disabled, stretch, letterbox, overscan, integer_scale };
struct logical_presentation_info { extent size; logical_presentation_mode mode; };
enum class texture_access { static_, streaming, target };
struct vertex { fpoint position; fcolor color; fpoint tex_coord; };   // layout of SDL_Vertex

auto render_driver_count() noexcept -> int;
auto render_driver(int index) noexcept -> std::string_view;           // empty if out of range

class renderer
{
  public:
    static auto create(window target, char const* driver = nullptr) -> std::expected<unique_renderer, error>;
    static auto create_software(surface target) -> std::expected<unique_renderer, error>;

    auto handle() const noexcept -> SDL_Renderer*;
    explicit operator bool() const noexcept;

    auto name() const -> std::expected<std::string_view, error>;
    auto get_window() const -> std::expected<window, error>;
    auto output_size() const -> std::expected<extent, error>;

    auto set_vsync(vsync_mode mode) const -> std::expected<void, error>;
    auto get_vsync() const -> std::expected<vsync_mode, error>;

    auto set_draw_color(rgba color) const -> std::expected<void, error>;
    auto set_draw_color(fcolor color) const -> std::expected<void, error>;
    auto draw_color() const -> std::expected<rgba, error>;
    auto set_draw_blend_mode(blend_mode mode) const -> std::expected<void, error>;
    auto get_draw_blend_mode() const -> std::expected<blend_mode, error>;
    auto set_viewport(std::optional<rect> area) const -> std::expected<void, error>;
    auto viewport() const -> std::expected<rect, error>;
    auto set_clip_rect(std::optional<rect> area) const -> std::expected<void, error>;
    auto clip_rect() const -> std::expected<rect, error>;
    auto clip_enabled() const noexcept -> bool;
    auto set_scale(fpoint scale) const -> std::expected<void, error>;
    auto scale() const -> std::expected<fpoint, error>;

    auto set_logical_presentation(extent size, logical_presentation_mode mode) const -> std::expected<void, error>;
    auto logical_presentation() const -> std::expected<logical_presentation_info, error>;
    auto coordinates_from_window(fpoint at) const -> std::expected<fpoint, error>;
    auto coordinates_to_window(fpoint at) const -> std::expected<fpoint, error>;
    auto convert_event_coordinates(event& e) const -> std::expected<void, error>;

    auto set_target(texture target) const -> std::expected<void, error>;
    auto reset_target() const -> std::expected<void, error>;
    auto target() const noexcept -> texture;

    auto clear() const -> std::expected<void, error>;
    auto present() const -> std::expected<void, error>;
    auto flush() const -> std::expected<void, error>;

    auto draw_point(fpoint at) const -> std::expected<void, error>;
    auto draw_points(std::span<fpoint const> points) const -> std::expected<void, error>;
    auto draw_line(fpoint from, fpoint to) const -> std::expected<void, error>;
    auto draw_lines(std::span<fpoint const> points) const -> std::expected<void, error>;
    auto draw_rect(frect area) const -> std::expected<void, error>;
    auto draw_rects(std::span<frect const> areas) const -> std::expected<void, error>;
    auto fill_rect(frect area) const -> std::expected<void, error>;
    auto fill_rects(std::span<frect const> areas) const -> std::expected<void, error>;

    auto draw_texture(texture source, std::optional<frect> from = {}, std::optional<frect> to = {}) const
        -> std::expected<void, error>;
    auto draw_texture_rotated(texture source, std::optional<frect> from, std::optional<frect> to,
                              double angle_degrees, std::optional<fpoint> center = {},
                              flip_mode flip = flip_mode::none) const -> std::expected<void, error>;
    auto draw_texture_tiled(texture source, std::optional<frect> from, float scale,
                            std::optional<frect> to) const -> std::expected<void, error>;
    auto draw_texture_9grid(texture source, std::optional<frect> from, float left, float right,
                            float top, float bottom, float scale, std::optional<frect> to) const
        -> std::expected<void, error>;

    auto draw_geometry(texture source, std::span<vertex const> vertices, std::span<int const> indices = {}) const
        -> std::expected<void, error>;
    auto draw_debug_text(fpoint at, char const* text) const -> std::expected<void, error>;
    auto read_pixels(std::optional<rect> area = {}) const -> std::expected<unique_surface, error>;
};

class texture
{
  public:
    static auto create(renderer owner, pixel_format format, texture_access access, extent size)
        -> std::expected<unique_texture, error>;
    static auto from_surface(renderer owner, surface source) -> std::expected<unique_texture, error>;

    auto handle() const noexcept -> SDL_Texture*;
    explicit operator bool() const noexcept;

    auto size() const noexcept -> extent;
    auto format() const noexcept -> pixel_format;

    auto update(std::optional<rect> area, std::span<std::byte const> pixels, int pitch) const
        -> std::expected<void, error>;
    auto lock(std::optional<rect> area = {}) const -> std::expected<texture_lock, error>;

    auto set_color_mod(rgb color) const -> std::expected<void, error>;
    auto color_mod() const -> std::expected<rgb, error>;
    auto set_alpha_mod(std::uint8_t alpha) const -> std::expected<void, error>;
    auto alpha_mod() const -> std::expected<std::uint8_t, error>;
    auto set_blend_mode(blend_mode mode) const -> std::expected<void, error>;
    auto get_blend_mode() const -> std::expected<blend_mode, error>;
    auto set_scale_mode(scale_mode mode) const -> std::expected<void, error>;
    auto get_scale_mode() const -> std::expected<scale_mode, error>;
};

class texture_lock   // move-only, unlocks when destroyed
{
  public:
    auto pixels() const noexcept -> std::span<std::byte>;
    auto pitch() const noexcept -> int;
};
```

Rules:

- **Frames.** Everything is drawn in floating point coordinates in the
  current logical space, after the logical presentation, the scale and the
  viewport. After `present` the back buffer is undefined: redraw every frame
  in full. Do not `present` while a render target texture is set.
- **Thread.** All render calls belong to the thread that created the
  renderer, usually the main thread.
- **Lifetimes.** The renderer must outlive its textures: destroying it
  invalidates them, so destroy the textures first. A rendering function
  called with a texture of another renderer fails. A window outlives its
  renderer, and a surface outlives the software renderer made from it. None
  of this is enforced.
- **Window or renderer.** A window has either a renderer or a window surface
  (`window::get_surface`), not both.
- **Null rects.** A null `from` is the whole texture and a null `to` is the
  whole render target.
- **`create_software`** makes a renderer with no window: `get_window` and
  `set_vsync` fail for it.
- **`set_vsync`** can fail on some drivers, and `adaptive` is not supported
  everywhere. Check the result.
- **`set_draw_color`** has an `rgba` and an `fcolor` overload, so a braced
  list is ambiguous: write `rgba{...}` or `fcolor{...}`. An `rgb` converts to
  an `rgba`. `draw_color` returns 8 bits per channel.
- **`convert_event_coordinates`** is for mouse and touch events. It changes
  the coordinates stored in the event, so the alternatives extracted from it
  afterwards carry logical coordinates, the ones extracted before do not.
- **`draw_points`, `draw_lines`, `draw_rects`, `fill_rects`** and
  `draw_geometry` with no elements succeed and draw nothing. Counts that do
  not fit an `int` fail. `draw_lines` is a connected polyline.
- **`draw_geometry`** with a null texture draws untextured triangles in the
  colors of the vertices. Without indices the vertices are taken three at a
  time.
- **`draw_debug_text`** uses SDL's built-in 8 by 8 pixel font, ASCII only, in
  the draw color, for tools and demos.
- **`read_pixels`** returns a new surface that the caller owns, of the
  viewport or the area of it, so the bars of a letterbox are not read. It is
  slow and stalls the pipeline; read before `present`.
- **`texture::create`** makes a texture of the pixel format, whose pixels are
  not initialized. Render target textures are created with
  `texture_access::target`. **`from_surface`** copies the pixels, the surface
  can be released afterwards.
- **`size` and `format`** read the public fields of `SDL_Texture`, so they do
  not fail.
- **`update`** fails when the span is smaller than the area, which for the
  FourCC formats is only checked for their first plane.
- **`lock`** is for streaming textures only. A `texture_lock` gives
  write-only access to the locked area, as SDL defines it, and its span is
  invalid once the lock is gone.

---

## Time

```cpp
struct clock
{
    using duration = std::chrono::nanoseconds;
    using rep = duration::rep;
    using period = duration::period;
    using time_point = std::chrono::time_point<clock>;
    static constexpr bool is_steady = true;
    static auto now() noexcept -> time_point;   // time since SDL was initialized
};

auto performance_counter() noexcept -> std::uint64_t;
auto performance_frequency() noexcept -> std::uint64_t;
void delay(std::chrono::nanoseconds duration) noexcept;
void delay_precise(std::chrono::nanoseconds duration) noexcept;
```

`clock` is a steady clock that satisfies `std::chrono::is_clock`. `rep` and
`period` are those of `duration`, which are `std::int64_t` and `std::nano`
wherever that is what `std::chrono::nanoseconds` is made of. A duration that
is not positive does not wait. `delay_precise` spends the final stretch
spinning, to be more accurate and at a higher cost of processor time. These
functions may be called from any thread.

---

## Keyboard

```cpp
enum class scancode : std::uint32_t { unknown, a, b, /* ... */ };
enum class keycode : std::uint32_t { unknown, return_, escape, /* ... */ };
enum class key_modifiers : std::uint16_t { none, left_shift, /* ... */ };

auto name(scancode code) noexcept -> std::string_view;
```

Generated from SDL's, with its values. `name` is SDL-owned and empty for
no name.

---

## Events

### Types

```cpp
enum class event_type : std::uint32_t { quit, key_down, /* ... */ };
```

One enumerator per SDL3 event type, in snake case without the prefix and
with SDL's values, plus `first`, `private0` to `private3`,
`poll_sentinel`, `user` and `last`, which are not event kinds. The
`*_first` and `*_last` aliases of SDL's ranges are not reproduced.

Each event type has an alternative named `<event_type>_event`, a small,
trivially copyable value that does not alias the `SDL_Event`.
`unknown_event` matches any other type that is not a user type, and
`user_event` matches `event_type::user` to `event_type::last`.

| Family | Alternatives | Accessors |
|--------|--------------|-----------|
| every one | all | `timestamp()` as `std::chrono::nanoseconds` |
| window | `window_shown_event`, `window_exposed_event`, `window_close_requested_event`, ... | `window_id()` |
| | `window_moved_event` | `window_id()`, `position()` as `point` |
| | `window_resized_event`, `window_pixel_size_changed_event` | `window_id()`, `size()` as `extent` |
| | `window_display_changed_event` | `window_id()`, `display()` as `display_id` |
| display | `display_added_event`, `display_removed_event`, `display_moved_event`, `display_orientation_event`, `display_desktop_mode_changed_event`, `display_current_mode_changed_event`, `display_content_scale_changed_event`, `display_usable_bounds_changed_event` | `display()` as `display_id` |
| pinch (SDL 3.4) | `pinch_begin_event`, `pinch_update_event`, `pinch_end_event` | `window_id()`, `scale()` |
| keyboard | `key_down_event`, `key_up_event` | `window_id()`, `which()`, `scancode()`, `keycode()`, `modifiers()`, `raw()`, `down()`, `repeat()` |
| | `keyboard_added_event`, `keyboard_removed_event` | `which()` as `keyboard_id` |
| mouse | `mouse_motion_event` | `window_id()`, `which()`, `buttons()`, `x()`, `y()`, `x_relative()`, `y_relative()` |
| | `mouse_button_down_event`, `mouse_button_up_event` | `window_id()`, `which()`, `button()`, `down()`, `clicks()`, `x()`, `y()` |
| | `mouse_wheel_event` | `window_id()`, `which()`, `x()`, `y()`, `direction()`, `mouse_x()`, `mouse_y()` |
| | `mouse_added_event`, `mouse_removed_event` | `which()` as `mouse_id` |
| `unknown_event` | | `type()` |
| `user_event` | | `type()`, `window_id()`, `code()`, `data1<T>()`, `data2<T>()` |

The other alternatives (display, joystick, gamepad, finger, pen, audio,
camera, sensor, render, clipboard, drop, text and application events)
have `timestamp()` only.

`keyboard_id`, `mouse_id` and `display_id` are strong ids like
`window_id`. `mouse_button` (`left`, `middle`, `right`, `x1`, `x2`) and
`mouse_button_flags` (the bit masks, with the bitwise operators) and
`mouse_wheel_direction` are `enum class` with SDL's values.

### `event`

```cpp
class event
{
  public:
    auto type() const noexcept -> event_type;

    template <typename E> auto holds_alternative() const noexcept -> bool;
    template <typename E> auto get_if() const noexcept -> std::optional<E>;
    template <typename E> auto get() const noexcept -> E;
    template <typename F> auto visit(F&& visitor) const -> decltype(auto);

    auto sdl() const noexcept -> SDL_Event const&;
    auto sdl() noexcept -> SDL_Event&;
};
```

A zero-initialized copy of the SDL event. `get` requires that
`holds_alternative<E>()`. `visit` calls the visitor with the alternative
that matches; it must be callable with all of them, and return one common
type.

### Queue functions

```cpp
void pump() noexcept;
auto poll(event& out) noexcept -> bool;
auto poll() -> std::optional<event>;
auto wait(event& out) -> std::expected<void, error>;
auto wait_timeout(event& out, std::chrono::milliseconds timeout) noexcept -> bool;
auto push(event const& e) -> std::expected<void, error>;
auto register_events(int count) -> std::expected<event_type, error>;
```

- **`pump`, `poll`, `wait`, `wait_timeout`** must be called from the main
  thread.
- **`poll`** returns `false` when the poll cycle is over, and then `out`
  is unspecified. SDL ends each cycle with an internal sentinel, so events
  pushed after the previous `false` may need one more `poll`.
- **`wait_timeout`** returns `false` on timeout and on error, SDL cannot
  tell them apart. A negative timeout waits indefinitely, a zero timeout
  does not wait, and the value is clamped to 32 bits.
- **`push`** copies the event, and can be called from any thread. An
  empty error message means that the event was rejected by the event
  filter.
- **`register_events`** returns the first of `count` consecutive event
  types for `user_event`. They are never released.

### Callbacks

```cpp
class event_watch;    // movable, removes the watch on destruction
class event_filter;   // movable, clears the filter on destruction

template <typename F> auto add_watch(F& callback) -> std::expected<event_watch, error>;
template <typename F> auto set_filter(F& callback) -> event_filter;
```

- **`add_watch`** calls `callback(event const&)` for every event that is
  pushed or generated, before it is queued.
- **`set_filter`** calls `bool callback(event const&)` for every event,
  and drops the ones that return `false`. SDL has one filter: setting it
  replaces the previous one, and drops the events that are already queued
  and that the callback rejects. The `event_filter` of a replaced filter
  does not clear the new one.

The callable is taken by reference, not copied or type erased: the caller
keeps it alive for as long as it is registered. Callbacks may be invoked
from any thread, and must not throw.

`event_watch` and `event_filter` have a constructor from the raw
`SDL_EventFilter` and user data, to adopt a registration that was made
with SDL directly.

### `EGGS_SDL_EVENT_TYPES`

```cpp
#define EGGS_SDL_EVENT_TYPES(X) X(quit, SDL_EVENT_QUIT, common) ...
```

The X macro with one row, `X(name, SDL value, family)`, for each event
type with an alternative.
