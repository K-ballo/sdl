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

## The demo

`example/window_demo.cpp` opens an 800 by 600 resizable window, repaints
its surface with a color derived from the pointer position, prints the
scancode name and modifiers of the keys, and exits on a quit request, a
close request or the escape key.
