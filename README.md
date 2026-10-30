# Eggs.SDL

**Eggs.SDL** is a header-only C++23 wrapper over SDL3: `std::expected` errors,
RAII owners and strong types, without adding abstraction, hidden state or
dependencies.

## Requirements

- SDL 3.2.0 or later (tested up to 3.4.18), found with `find_package(SDL3 CONFIG)`
- CMake 4.0+ to build from source; CMake 3.21+ to consume an installed package via `find_package`
- A C++23 compiler with `std::expected`; CI covers GCC 15-16, Clang 21-22 (libstdc++ and libc++), MSVC 2022 and 2026, clang-cl and Apple Clang

See [docs/cmake.md](docs/cmake.md) for building SDL3 and pointing CMake at it.

## Building

```sh
export SDL3_ROOT=$HOME/.cache/sdl/prefix
cmake --preset dev-gcc -B build/dev-gcc
cmake --build build/dev-gcc --config Debug
ctest --test-dir build/dev-gcc --build-config Debug
```

Replace `dev-gcc` with `dev-clang`, `dev-clang-libcxx`, `dev-msvc` or `dev-clang-cl` as appropriate.

## Usage

```cpp
#include <eggs/sdl.hpp>

namespace sdl = eggs::sdl;

int main()
{
    auto const subsystems = sdl::init(sdl::init_flags::video);
    if (!subsystems)
        return 1;

    auto const window = sdl::window::create(
        "Hello", 800, 600, sdl::window_flags::resizable
    );
    if (!window)
        return 1;

    for (sdl::event event; sdl::wait(event);) {
        if (event.holds_alternative<sdl::quit_event>())
            break;
    }
}
```

`example/window_demo.cpp` opens a window and paints its surface from the pointer position.
`example/surface_demo.cpp` blits a moving sprite, drawn or loaded from a BMP file, onto the window surface.
`example/render_demo.cpp` draws sprites, shapes and geometry with the 2D renderer, at a logical size.

## What it provides

Error handling, init and quit, windows, the full set of SDL3 events, surfaces
with their pixel formats, blending and raw pixel access, float geometry and
color value types, the 2D renderer with textures, and time. See [docs](docs/index.md) for the design and the
reference.

## License

Boost Software License 1.0, see [LICENSE.txt](LICENSE.txt).
