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
| `<eggs/sdl/surface.hpp>` | `rgb`, `surface` |
| `<eggs/sdl/keyboard.hpp>` | `scancode`, `keycode`, `key_modifiers`, `name` |
| `<eggs/sdl/event.hpp>` | `event_type`, the event alternatives, `event`, the queue functions, `event_watch`, `event_filter` |

Headers under `eggs/sdl/detail/` are implementation details, except for
the `EGGS_SDL_EVENT_TYPES` table.

The SDL version required is 3.2.0. Every function that returns
`std::expected<T, error>` is `[[nodiscard]]`.

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

---

## Surface

```cpp
class surface
{
  public:
    constexpr surface() noexcept = default;
    constexpr surface(std::nullptr_t) noexcept;
    explicit constexpr surface(SDL_Surface* handle) noexcept;

    constexpr auto handle() const noexcept -> SDL_Surface*;
    constexpr explicit operator bool() const noexcept;

    auto size() const noexcept -> extent;
    auto fill(rgb color) const -> std::expected<void, error>;
    auto fill(rect area, rgb color) const -> std::expected<void, error>;
};
```

In stage 1 a surface is only ever the window surface, borrowed from its
window (see `window::get_surface` for its validity). There is no
`unique_surface` yet.

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
