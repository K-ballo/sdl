<!--
Eggs.SDL

Copyright (c) 2026 Agustin Berge

Distributed under the Boost Software License, Version 1.0. (See
accompanying file LICENSE.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
-->

# Design

## SDL3 with C++ types

The wrapper follows SDL3's names and structure: `SDL_CreateWindow` is
`window::create`, `SDL_GetWindowSurface` is `window::get_surface`,
`SDL_EVENT_KEY_DOWN` is `event_type::key_down`. There is no abstraction
layer, no hidden state, no global registry of objects and no
dependencies beyond SDL3 and the standard library. What the wrapper adds
is types.

## Handles and owners

Every SDL object type `T` comes as two types:

- `T` (`window`, `surface`) is a handle. It is a small, trivially
  copyable, pointer-sized wrapper of the SDL pointer, that is null when
  default constructed. It owns nothing, so functions that merely use an
  object take it by value. `handle()` returns the SDL pointer.
- `unique_T` (`unique_window`, `unique_surface`, `unique_subsystems`) is a
  `std::unique_ptr<T, T_deleter>` whose deleter declares `pointer` to be
  the handle. `get()` yields the handle and `->` forwards to its member
  functions.

Factories are `T::create(...) -> std::expected<unique_T, error>` and
never return an empty owner. Because owners are plain `std::unique_ptr`,
there is nothing to learn about their moves, resets or releases.

`native_handle()` is reserved for platform objects (`HWND`,
`NSWindow*`) and is not provided yet.

The handles have explicit constructors from the raw SDL pointers, to
adopt objects that were created elsewhere. The handle member functions
assert that the handle is not null in debug builds; that is a
precondition, not a runtime check.

## Lifetimes are documented, not enforced

A `surface` borrowed from a window, the span of a `surface_lock`, a `window` obtained with
`window::from_id`, a `std::string_view` of an SDL-owned string: none of
them keeps its source alive or notices when it dies. Each function
documents what the lifetime of its result depends on. There is no shared
ownership or reference counting anywhere in the library.

The window surface is the case worth knowing: it belongs to the window,
and becomes invalid when it is destroyed with `destroy_surface`, when the
window is destroyed, and when the window is resized or its pixel size
changes. Re-acquire it with `get_surface` on `window_resized_event`,
`window_pixel_size_changed_event` and `window_exposed_event` -- or simply
on every frame, `get_surface` is cheap when the surface is still valid.

## Errors

Every fallible call returns `std::expected<T, error>`, and no function
throws. `error::current()` captures `SDL_GetError()` right after the
failing call. The message is the only allocation the library ever makes,
and only on the failure path.

Where SDL cannot tell failure modes apart, the function returns `bool`
and says why in its documentation: `poll` (no event), `wait_timeout`
(timeout or error). `push` is the notable `expected` that can fail
without a message: an event that the event filter rejects is not an
error for SDL, but `SDL_PushEvent` reports it as a failure with the error
string cleared, so `push` returns an `error` with an empty message.

## Ids, enumerations and flags

Window and device ids are strong types (`window_id`, `keyboard_id`,
`mouse_id`, `display_id`), 0 being invalid. Enumerations and flag sets
are `enum class` with SDL's values, and the flag types get the bitwise
operators. The enumerators of `scancode` and `keycode` are SDL's names
in snake case without the prefix, with a trailing underscore where the
name is a C++ keyword (`return_`, `delete_`) and a `digit_` prefix for
the digits (`digit_1`).

SDL timestamps are `std::chrono::nanoseconds`. Strings that go to SDL
are `char const*`, because SDL needs them NUL-terminated; strings that
come from SDL are `std::string_view` of the SDL-owned memory.

## Subsystems

`init` returns a `unique_subsystems`, an owner of the flags it
initialized. Destroying it calls `SDL_QuitSubSystem` for exactly those
flags, never `SDL_Quit`, so owners nest and libraries can init video
without knowing about each other. `quit()` is the escape hatch that
calls `SDL_Quit`; owners are not aware of it.

Since the flags are not a pointer, the owner's `pointer` is `subsystems`,
a pointer-sized value that holds them, so that the owner is itself
pointer-sized.

## Events

`event_type` has an enumerator for every SDL3 event type, and every one
of them has an alternative type named `<event_type>_event`. They are
small, trivially copyable values that copy the fields out of the SDL
event, so they are safe to store: they never alias it. Fields become
typed accessors: timestamps are `std::chrono::nanoseconds`, ids are
strong types, enumerated fields are `enum class`, flags are `bool`,
coordinates stay `float`.

An `event` is a copy of the `SDL_Event`, and offers the same access as
`std::variant`: `holds_alternative`, `get_if`, `get` and `visit`. Two
alternatives live outside the scheme: `unknown_event` for any type that is
not covered, including types that newer SDL3 releases add, and
`user_event` for the types handed out by `register_events`.

All alternatives have `timestamp()`. The keyboard, mouse and window
families have the accessors for all their fields. The other families
have only `timestamp()` for now. The text of `text_input_event`,
`text_editing_event` and the drop events is not exposed: SDL owns it
only until the next poll, and an alternative never aliases the event.

The table of event types is one X macro, `EGGS_SDL_EVENT_TYPES`, in
`eggs/sdl/detail/event_types.hpp`. Each row maps a name and its SDL value
to the family that gives the accessors, so supporting a new SDL event is
adding a row.

### Callbacks

`add_watch` and `set_filter` take the callable by reference. The caller
keeps it alive for as long as it is registered, and there is no type
erasure and no allocation. The thunk is a function template instance, and
the callable is the user data. SDL may call them from any thread. A
callback must not throw: the thunks are `noexcept`.

SDL has one event filter. `set_filter` replaces the previous one, and
the `event_filter` of the replaced filter does not clear the new one when
it is destroyed.

## Platform thread rules

Video, window and event pumping functions must be called from the thread
that initialized video, usually the main thread. `push`, `register_events`,
`add_watch` and `set_filter` can be called from any thread.
