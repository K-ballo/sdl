<!--
Eggs.SDL

Copyright (c) 2026 Agustin Berge

Distributed under the Boost Software License, Version 1.0. (See
accompanying file LICENSE.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
-->

# Introduction

**Eggs.SDL** is a header-only C++23 wrapper over SDL3. It adds type
safety, RAII and `std::expected` errors to SDL3's C API, without adding
abstraction, hidden state or dependencies. It is meant to read as "SDL3
with C++ types", not as a framework.

## Why

SDL3 is a C API: pointers that may be null, `bool` results with the
reason in a global error string, integers for ids, and flags as macros.
Eggs.SDL keeps the same shape and the same names, and changes only the
types:

- objects are small handles, with an owning `std::unique_ptr` when you
  want RAII;
- every fallible call returns `std::expected<T, eggs::sdl::error>`;
- ids, flags and enumerations are `enum class` with SDL's values;
- durations are `std::chrono`;
- events are a closed set of small value types that can be matched with
  `visit`.

## What it provides

- **`<eggs/sdl/error.hpp>`** -- `error`.
- **`<eggs/sdl/init.hpp>`** -- `init`, `quit`, `was_init`, `unique_subsystems`
  and the application metadata functions.
- **`<eggs/sdl/window.hpp>`** -- `window`, `unique_window` and the window
  surface functions.
- **`<eggs/sdl/surface.hpp>`** -- `surface`, `unique_surface` and
  `surface_lock`: surfaces that are created, loaded or borrowed from a
  window, with blits, fills, conversions and raw pixel access.
- **`<eggs/sdl/geometry.hpp>`, `color.hpp`, `pixel_format.hpp`,
  `blend_mode.hpp`** -- the value types and enumerations of the above.
- **`<eggs/sdl/time.hpp>`** -- `clock`, `delay` and the performance counter.
- **`<eggs/sdl/keyboard.hpp>`** -- `scancode`, `keycode`, `key_modifiers`.
- **`<eggs/sdl/event.hpp>`** -- the full set of SDL3 events, the event
  queue functions, watches and filters.
- **`<eggs/sdl.hpp>`** -- all of the above.

Renderer, GPU, audio, input state queries, properties and the other SDL
subsystems are not wrapped yet.

## A window in a few lines

```cpp
#include <eggs/sdl.hpp>

namespace sdl = eggs::sdl;

int main()
{
    auto const subsystems = sdl::init(sdl::init_flags::video);
    if (!subsystems)
        return 1;

    auto const window = sdl::window::create("Hello", 800, 600);
    if (!window)
        return 1;

    for (sdl::event event; sdl::wait(event);) {
        if (event.holds_alternative<sdl::quit_event>())
            break;
    }
}
```

## Requirements

- A C++23 compiler with `std::expected`; CI covers GCC 15-16, Clang 21-22
  (libstdc++ and libc++), MSVC 2022 and 2026, clang-cl and Apple Clang
- SDL 3.2.0 or later (tested up to 3.4.18), found with
  `find_package(SDL3 CONFIG)`
- CMake 4.0+ to build from source; CMake 3.21+ to consume an installed
  package via `find_package`
