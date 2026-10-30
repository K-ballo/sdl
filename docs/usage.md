<!--
Eggs.SDL

Copyright (c) 2026 Agustin Berge

Distributed under the Boost Software License, Version 1.0. (See
accompanying file LICENSE.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
-->

# Usage

## Initializing

```cpp
namespace sdl = eggs::sdl;

auto const subsystems = sdl::init(sdl::init_flags::video);
if (!subsystems) {
    std::print(stderr, "{}\n", subsystems.error().message());
    return EXIT_FAILURE;
}
```

`init` returns an owner. When it goes out of scope the subsystems it
initialized are released. Video implies events, and gamepad implies
joystick. Video must be initialized, and used, from the main thread.

Flags are combined with `|`:

```cpp
auto const flags = sdl::init_flags::video | sdl::init_flags::audio;
```

Application metadata is set before creating windows:

```cpp
auto const metadata = sdl::set_app_metadata("My App", "1.0.0", "org.example.my-app");
```

## Windows and the window surface

```cpp
auto owned = sdl::window::create(
    "My App", 800, 600, sdl::window_flags::resizable
);
if (!owned)
    return fail(owned.error());

sdl::window const window = owned->get();
```

`window` is a handle; pass it by value to functions that use the window,
and keep the `unique_window` where the window should die. The member
functions can also be called through the owner (`(*owned)->show()`).

To paint on the CPU, acquire the window surface, fill it and present:

```cpp
auto paint(sdl::window window, sdl::rgb color) -> std::expected<void, sdl::error>
{
    return window.get_surface()
        .and_then([&](sdl::surface surface) { return surface.fill(color); })
        .and_then([&] { return window.update_surface(); });
}
```

The surface is borrowed from the window. It is invalid after the window
is resized or its pixel size changes, so acquire it again each time you
paint.

## Events

```cpp
sdl::event event;
while (running) {
    if (auto const waited = sdl::wait(event); !waited)
        return fail(waited.error());

    event.visit(overloaded{
        [&](sdl::quit_event const&) { running = false; },
        [&](sdl::key_down_event const& key) {
            if (key.scancode() == sdl::scancode::escape)
                running = false;
        },
        [&](sdl::mouse_motion_event const& motion) { /* motion.x(), motion.y() */ },
        [](auto const&) {},
    });
}
```

`visit` calls the visitor with the alternative that matches, so it must
accept all of them, hence the final generic overload. When only one or
two types matter, `holds_alternative`, `get_if` and `get` are lighter:

```cpp
if (auto const key = event.get_if<sdl::key_up_event>())
    use(key->keycode(), key->modifiers());
```

`poll` returns `false` when there is nothing more to do right now. SDL
ends each poll cycle with an internal sentinel, so an event that was
pushed after a `false` return can take one more `poll` to show up.
`wait_timeout` returns `false` on timeout and on error alike.

### User events

```cpp
auto const first = sdl::register_events(1);
if (!first)
    return fail(first.error());

sdl::event event;
event.sdl().user.type = static_cast<std::uint32_t>(*first);
event.sdl().user.code = 7;
auto const pushed = sdl::push(event);
```

The event comes back as a `user_event`, with `type()`, `code()` and the
`data1<T>()` and `data2<T>()` pointers. `sdl()` gives access to the
underlying `SDL_Event` for everything the wrapper does not cover.

### Watches and filters

```cpp
auto log = [](sdl::event const& e) { /* any thread */ };
auto watch = sdl::add_watch(log);   // removed when `watch` is destroyed

auto drop_motion = [](sdl::event const& e) {
    return !e.holds_alternative<sdl::mouse_motion_event>();
};
sdl::event_filter filter = sdl::set_filter(drop_motion);
```

The callables must outlive the watch or filter that refers to them, and
must not throw.

## Surfaces

A surface of your own is a `unique_surface`, which destroys it. It can be
drawn on, converted and blitted, onto another surface or onto the window
surface:

```cpp
auto sprite = sdl::surface::create({.width = 64, .height = 64}, sdl::pixel_format::rgba32);
if (!sprite)
    return 1;
(void)(*sprite)->fill({.x = 8, .y = 8, .width = 16, .height = 16}, {.r = 255, .g = 0, .b = 0});

// Converted once to the format it is blitted to, which makes blits faster.
if (auto converted = (*sprite)->convert(window_surface.format()))
    (void)(*converted)->blit(window_surface, {.x = 100, .y = 50});
```

Surfaces of different pixel formats can be blitted, SDL converts as it goes.
Pixel memory is reached through a `surface_lock`, which is only valid for as
long as it lives:

```cpp
if (auto locked = surface.lock()) {
    std::span<std::byte> pixels = locked->pixels();
    pixels[0] = std::byte{255};
}   // unlocked here, `pixels` is no longer valid
```

Time comes from `sdl::clock`, which satisfies the standard clock
requirements, and `sdl::delay`:

```cpp
auto const frame_start = sdl::clock::now();
// ... draw ...
sdl::delay(std::chrono::milliseconds{16} - (sdl::clock::now() - frame_start));
```

## The renderer

To draw on the GPU, or with SDL's software renderer, create a renderer for
the window. A window has either a renderer or a window surface, not both. The
renderer must outlive its textures, so declare them after it:

```cpp
auto owned = sdl::renderer::create(window);
if (!owned)
    return fail(owned.error());
sdl::renderer const renderer = owned->get();

(void)renderer.set_vsync(sdl::vsync_mode::every_refresh);   // can fail on some drivers
(void)renderer.set_logical_presentation(
    {.width = 320, .height = 180}, sdl::logical_presentation_mode::letterbox
);

auto sprite = sdl::texture::from_surface(renderer, *surface);   // copies the pixels
```

Every frame is drawn in full and presented, the back buffer is undefined
afterwards:

```cpp
(void)renderer.set_draw_color(sdl::rgba{.r = 20, .g = 24, .b = 40});
(void)renderer.clear();
(void)renderer.draw_texture(sprite->get(), {}, sdl::frect{.x = 10, .y = 10, .width = 32, .height = 32});
(void)renderer.fill_rect({.x = 50, .y = 10, .width = 40, .height = 20});
(void)renderer.draw_debug_text({.x = 8, .y = 8}, "hello");
(void)renderer.present();
```

Name the type in `set_draw_color`, `rgba{...}` or `fcolor{...}`: a braced
list fits both. Mouse events are in window coordinates, convert them to the
logical ones before extracting the alternative:

```cpp
if (event.holds_alternative<sdl::mouse_motion_event>())
    (void)renderer.convert_event_coordinates(event);
if (auto const motion = event.get_if<sdl::mouse_motion_event>())
    use(motion->x(), motion->y());
```

A render target texture is created with `texture_access::target`, drawn on
between `set_target` and `reset_target`, and then drawn like any texture.
A streaming texture is written through a `texture_lock`, whose span is
invalid once the lock is gone. `read_pixels` returns what was drawn as a new
surface, which `save_bmp` can write.

## The demos

`example/window_demo.cpp` opens an 800 by 600 resizable window, repaints
its surface with a color derived from the pointer position, prints the
scancode name and modifiers of the keys, and exits on a quit request, a
close request or the escape key.

`example/surface_demo.cpp` draws a 64 by 64 sprite, or loads the BMP file
that is its first argument, converts it once to the format of the window
surface and blits it every frame at a position that bounces with
`sdl::clock::now()`, and once more scaled. The S key saves the window to
`frame.bmp`, and the loop keeps to about 60 frames per second with
`sdl::delay`. It exits on a quit request, a close request or the escape key.

`example/render_demo.cpp` opens a resizable, high pixel density window with a
renderer, vsync and a logical size of 320 by 180 with letterboxing. It draws
a checkerboard texture, or the BMP file that is its first argument, bouncing
and rotating, rectangles, lines, a triangle from `draw_geometry` with three
vertex colors and the frames per second. The mouse position is converted to
logical coordinates for a crosshair, the space key cycles the presentation
between `letterbox` and `integer_scale`, and the S key saves a screenshot to
`screenshot.bmp` with `read_pixels`. It exits on a quit request, a close
request or the escape key.
