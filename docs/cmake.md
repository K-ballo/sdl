<!--
Eggs.SDL

Copyright (c) 2026 Agustin Berge

Distributed under the Boost Software License, Version 1.0. (See
accompanying file LICENSE.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
-->

# CMake Integration

## SDL3

Eggs.SDL needs SDL 3.2.0 or later, found with
`find_package(SDL3 3.2.0 CONFIG REQUIRED)`. If SDL3 is not in a default
location, point CMake at its install prefix, through `CMAKE_PREFIX_PATH`
or the `SDL3_ROOT` environment variable:

```sh
git clone --depth 1 --branch release-3.2.x https://github.com/libsdl-org/SDL.git
cmake -S SDL -B SDL/build -DCMAKE_BUILD_TYPE=Release -DSDL_TESTS=OFF \
    -DCMAKE_INSTALL_PREFIX=$HOME/.cache/sdl/prefix
cmake --build SDL/build
cmake --install SDL/build
export SDL3_ROOT=$HOME/.cache/sdl/prefix
```

On a machine without a display server or its development packages, such
as a CI runner, configure SDL with `-DSDL_UNIX_CONSOLE_BUILD=ON`. The
tests use SDL's dummy video driver, so they do not need a display.

## Building

```sh
cmake -S . -B build
cmake --build build
cmake --install build --prefix /usr/local
```

The project ships `CMakePresets.json` with configurations for GCC,
Clang, Clang with libc++, MSVC, clang-cl, and AddressSanitizer builds.
List available presets with:

```sh
cmake --list-presets
cmake --preset dev-gcc -B build/dev-gcc
cmake --build build/dev-gcc --config Debug
ctest --test-dir build/dev-gcc --build-config Debug
```

### Options

Pass options at configure time with `-D<option>=<value>`.

| Option | Subdirectory variant | Description |
|--------|----------------------|-------------|
| **`BUILD_DOCS`** (default: `ON`) | **`EGGS_SDL_BUILD_DOCS`** (default: `OFF`) | Build the documentation |
| **`BUILD_EXAMPLES`** (default: `ON`) | **`EGGS_SDL_BUILD_EXAMPLES`** (default: `OFF`) | Build the example programs |
| **`BUILD_TESTING`** (default: `ON`, via CTest) | **`EGGS_SDL_BUILD_TESTING`** (default: `OFF`) | Build the test suite |
| **`ENABLE_INSTALL`** (default: `ON`) | **`EGGS_SDL_ENABLE_INSTALL`** (default: `OFF`) | Generate install rules |

Building the documentation requires a Python 3 interpreter and network
access to install the pinned `markdown` and `pygments` packages into the
build tree. Configure with `-DBUILD_DOCS=OFF` to skip it.

The prefixed variants are used when the project is included via
`add_subdirectory` or FetchContent, so that the examples and the tests of
a dependency are not built by default.

## Targets

| Target | Description |
|--------|-------------|
| **`Eggs::SDL`** | The header-only library. Requires C++23, links `SDL3::SDL3` |

## Consuming

### FetchContent

```cmake
include(FetchContent)
FetchContent_Declare(
    Eggs.SDL
    GIT_REPOSITORY https://github.com/eggs-cpp/sdl.git
    GIT_TAG        v@VERSION@
)
FetchContent_MakeAvailable(Eggs.SDL)

target_link_libraries(app PRIVATE Eggs::SDL)
```

### Installed package

```cmake
find_package(Eggs.SDL REQUIRED)
target_link_libraries(app PRIVATE Eggs::SDL)
```

The package finds SDL3 itself, with `find_dependency`.

## Tests

| Label | Content |
|-------|---------|
| `header` | Every header compiles on its own, and defines nothing twice |
| `unit` | The unit tests, run with `SDL_VIDEODRIVER=dummy` |
| `example` | The examples that need no display |

`test/window_demo.cpp` includes the demo and drives its event loop from
a thread through the three exit paths.
