// Eggs.SDL
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <eggs/sdl/detail/bitmask.hpp>
#include <eggs/sdl/detail/event_types.hpp> // IWYU pragma: export
#include <eggs/sdl/detail/geometry.hpp>
#include <eggs/sdl/detail/keycode.hpp>
#include <eggs/sdl/detail/scancode.hpp>
#include <eggs/sdl/error.hpp>
#include <eggs/sdl/keyboard.hpp>
#include <eggs/sdl/window.hpp>

#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <chrono>
#include <concepts>
#include <cstdint>
#include <expected>
#include <limits>
#include <memory>
#include <optional>
#include <utility>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_stdinc.h>

namespace eggs::sdl {

// 0 is invalid for all of the ids.
enum class keyboard_id : std::uint32_t
{
};

enum class mouse_id : std::uint32_t
{
};

enum class display_id : std::uint32_t
{
};

enum class mouse_button : std::uint8_t
{
    none = 0,
    left = SDL_BUTTON_LEFT,
    middle = SDL_BUTTON_MIDDLE,
    right = SDL_BUTTON_RIGHT,
    x1 = SDL_BUTTON_X1,
    x2 = SDL_BUTTON_X2,
};

enum class mouse_button_flags : std::uint32_t
{
    none = 0,
    left = SDL_BUTTON_LMASK,
    middle = SDL_BUTTON_MMASK,
    right = SDL_BUTTON_RMASK,
    x1 = SDL_BUTTON_X1MASK,
    x2 = SDL_BUTTON_X2MASK,
};

template <>
inline constexpr bool enable_bitmask_operators<mouse_button_flags> = true;

enum class mouse_wheel_direction : std::uint32_t
{
    normal = SDL_MOUSEWHEEL_NORMAL,
    flipped = SDL_MOUSEWHEEL_FLIPPED,
};

// The enumerators from first to last are not event kinds: they are range
// bounds and SDL private values, which are all reported as unknown_event or
// user_event.
enum class event_type : std::uint32_t
{
#define EGGS_SDL_EVENT_ENUMERATOR(name, value, family) name = value,
    EGGS_SDL_EVENT_TYPES(EGGS_SDL_EVENT_ENUMERATOR)
#undef EGGS_SDL_EVENT_ENUMERATOR
        first = SDL_EVENT_FIRST,
    private0 = SDL_EVENT_PRIVATE0,
    private1 = SDL_EVENT_PRIVATE1,
    private2 = SDL_EVENT_PRIVATE2,
    private3 = SDL_EVENT_PRIVATE3,
    poll_sentinel = SDL_EVENT_POLL_SENTINEL,
    user = SDL_EVENT_USER,
    last = SDL_EVENT_LAST,
};

namespace detail {

[[nodiscard]] constexpr auto is_user_type(event_type type) noexcept -> bool
{
    return type >= event_type::user && type <= event_type::last;
}

// The case lists are generated from the event table; default covers the
// enumerators that are not events (first, last, user and so on).
#ifdef __clang__
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wswitch-enum"
#endif

[[nodiscard]] constexpr auto is_known_type(event_type type) noexcept -> bool
{
    switch (type) {
#define EGGS_SDL_EVENT_KNOWN(name, value, family) case event_type::name:
        EGGS_SDL_EVENT_TYPES(EGGS_SDL_EVENT_KNOWN)
#undef EGGS_SDL_EVENT_KNOWN
        return true;
        default:
            return false;
    }
}

#ifdef __clang__
#    pragma clang diagnostic pop
#endif

[[nodiscard]] constexpr auto to_nanoseconds(std::uint64_t value) noexcept
    -> std::chrono::nanoseconds
{
    return std::chrono::nanoseconds(
        static_cast<std::chrono::nanoseconds::rep>(value)
    );
}

// The event classes hold their fields in the order of the SDL events, so the
// padding that -Wpadded reports is expected.
#ifdef __clang__
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wpadded"
#endif

// Names such as window_id() and scancode() are also types, so inside the
// event classes the types are always spelled with a namespace qualifier.

template <event_type T>
class common_event
{
  public:
    static constexpr event_type type = T;

    [[nodiscard]] static constexpr auto matches(event_type value) noexcept
        -> bool
    {
        return value == T;
    }

    constexpr common_event() noexcept = default;

    explicit common_event(SDL_Event const& e) noexcept
        : timestamp_(to_nanoseconds(e.common.timestamp))
    {
    }

    [[nodiscard]] constexpr auto timestamp() const noexcept
        -> std::chrono::nanoseconds
    {
        return timestamp_;
    }

  private:
    std::chrono::nanoseconds timestamp_{};
};

template <event_type T>
class window_event : public common_event<T>
{
  public:
    constexpr window_event() noexcept = default;

    explicit window_event(SDL_Event const& e) noexcept
        : common_event<T>(e),
          window_id_(static_cast<eggs::sdl::window_id>(e.window.windowID))
    {
    }

    [[nodiscard]] constexpr auto window_id() const noexcept
        -> eggs::sdl::window_id
    {
        return window_id_;
    }

  private:
    eggs::sdl::window_id window_id_{};
};

template <event_type T>
class window_position_event : public window_event<T>
{
  public:
    constexpr window_position_event() noexcept = default;

    explicit window_position_event(SDL_Event const& e) noexcept
        : window_event<T>(e),
          position_{.x = e.window.data1, .y = e.window.data2}
    {
    }

    [[nodiscard]] constexpr auto position() const noexcept -> point
    {
        return position_;
    }

  private:
    point position_{};
};

template <event_type T>
class window_size_event : public window_event<T>
{
  public:
    constexpr window_size_event() noexcept = default;

    explicit window_size_event(SDL_Event const& e) noexcept
        : window_event<T>(e),
          size_{.width = e.window.data1, .height = e.window.data2}
    {
    }

    [[nodiscard]] constexpr auto size() const noexcept -> extent
    {
        return size_;
    }

  private:
    extent size_{};
};

template <event_type T>
class window_display_event : public window_event<T>
{
  public:
    constexpr window_display_event() noexcept = default;

    explicit window_display_event(SDL_Event const& e) noexcept
        : window_event<T>(e),
          display_(static_cast<display_id>(e.window.data1))
    {
    }

    [[nodiscard]] constexpr auto display() const noexcept -> display_id
    {
        return display_;
    }

  private:
    display_id display_{};
};

template <event_type T>
class keyboard_device_event : public common_event<T>
{
  public:
    constexpr keyboard_device_event() noexcept = default;

    explicit keyboard_device_event(SDL_Event const& e) noexcept
        : common_event<T>(e),
          which_(static_cast<keyboard_id>(e.kdevice.which))
    {
    }

    [[nodiscard]] constexpr auto which() const noexcept -> keyboard_id
    {
        return which_;
    }

  private:
    keyboard_id which_{};
};

template <event_type T>
class mouse_device_event : public common_event<T>
{
  public:
    constexpr mouse_device_event() noexcept = default;

    explicit mouse_device_event(SDL_Event const& e) noexcept
        : common_event<T>(e),
          which_(static_cast<mouse_id>(e.mdevice.which))
    {
    }

    [[nodiscard]] constexpr auto which() const noexcept -> mouse_id
    {
        return which_;
    }

  private:
    mouse_id which_{};
};

template <event_type T>
class key_event : public common_event<T>
{
  public:
    constexpr key_event() noexcept = default;

    explicit key_event(SDL_Event const& e) noexcept
        : common_event<T>(e),
          window_id_(static_cast<eggs::sdl::window_id>(e.key.windowID)),
          which_(static_cast<keyboard_id>(e.key.which)),
          scancode_(static_cast<eggs::sdl::scancode>(e.key.scancode)),
          keycode_(static_cast<eggs::sdl::keycode>(e.key.key)),
          modifiers_(static_cast<key_modifiers>(e.key.mod)),
          raw_(e.key.raw),
          down_(e.key.down),
          repeat_(e.key.repeat)
    {
    }

    [[nodiscard]] constexpr auto window_id() const noexcept
        -> eggs::sdl::window_id
    {
        return window_id_;
    }

    [[nodiscard]] constexpr auto which() const noexcept -> keyboard_id
    {
        return which_;
    }

    [[nodiscard]] constexpr auto scancode() const noexcept
        -> eggs::sdl::scancode
    {
        return scancode_;
    }

    [[nodiscard]] constexpr auto keycode() const noexcept -> eggs::sdl::keycode
    {
        return keycode_;
    }

    [[nodiscard]] constexpr auto modifiers() const noexcept -> key_modifiers
    {
        return modifiers_;
    }

    [[nodiscard]] constexpr auto raw() const noexcept -> std::uint16_t
    {
        return raw_;
    }

    [[nodiscard]] constexpr auto down() const noexcept -> bool { return down_; }

    [[nodiscard]] constexpr auto repeat() const noexcept -> bool
    {
        return repeat_;
    }

  private:
    eggs::sdl::window_id window_id_{};
    keyboard_id which_{};
    eggs::sdl::scancode scancode_{};
    eggs::sdl::keycode keycode_{};
    key_modifiers modifiers_{};
    std::uint16_t raw_ = 0;
    bool down_ = false;
    bool repeat_ = false;
};

template <event_type T>
class mouse_motion_event : public common_event<T>
{
  public:
    constexpr mouse_motion_event() noexcept = default;

    explicit mouse_motion_event(SDL_Event const& e) noexcept
        : common_event<T>(e),
          window_id_(static_cast<eggs::sdl::window_id>(e.motion.windowID)),
          which_(static_cast<mouse_id>(e.motion.which)),
          buttons_(static_cast<mouse_button_flags>(e.motion.state)),
          x_(e.motion.x),
          y_(e.motion.y),
          x_relative_(e.motion.xrel),
          y_relative_(e.motion.yrel)
    {
    }

    [[nodiscard]] constexpr auto window_id() const noexcept
        -> eggs::sdl::window_id
    {
        return window_id_;
    }

    [[nodiscard]] constexpr auto which() const noexcept -> mouse_id
    {
        return which_;
    }

    [[nodiscard]] constexpr auto buttons() const noexcept -> mouse_button_flags
    {
        return buttons_;
    }

    [[nodiscard]] constexpr auto x() const noexcept -> float { return x_; }

    [[nodiscard]] constexpr auto y() const noexcept -> float { return y_; }

    [[nodiscard]] constexpr auto x_relative() const noexcept -> float
    {
        return x_relative_;
    }

    [[nodiscard]] constexpr auto y_relative() const noexcept -> float
    {
        return y_relative_;
    }

  private:
    eggs::sdl::window_id window_id_{};
    mouse_id which_{};
    mouse_button_flags buttons_ = mouse_button_flags::none;
    float x_ = 0;
    float y_ = 0;
    float x_relative_ = 0;
    float y_relative_ = 0;
};

template <event_type T>
class mouse_button_event : public common_event<T>
{
  public:
    constexpr mouse_button_event() noexcept = default;

    explicit mouse_button_event(SDL_Event const& e) noexcept
        : common_event<T>(e),
          window_id_(static_cast<eggs::sdl::window_id>(e.button.windowID)),
          which_(static_cast<mouse_id>(e.button.which)),
          button_(static_cast<mouse_button>(e.button.button)),
          down_(e.button.down),
          clicks_(e.button.clicks),
          x_(e.button.x),
          y_(e.button.y)
    {
    }

    [[nodiscard]] constexpr auto window_id() const noexcept
        -> eggs::sdl::window_id
    {
        return window_id_;
    }

    [[nodiscard]] constexpr auto which() const noexcept -> mouse_id
    {
        return which_;
    }

    [[nodiscard]] constexpr auto button() const noexcept -> mouse_button
    {
        return button_;
    }

    [[nodiscard]] constexpr auto down() const noexcept -> bool { return down_; }

    [[nodiscard]] constexpr auto clicks() const noexcept -> std::uint8_t
    {
        return clicks_;
    }

    [[nodiscard]] constexpr auto x() const noexcept -> float { return x_; }

    [[nodiscard]] constexpr auto y() const noexcept -> float { return y_; }

  private:
    eggs::sdl::window_id window_id_{};
    mouse_id which_{};
    mouse_button button_ = mouse_button::none;
    bool down_ = false;
    std::uint8_t clicks_ = 0;
    float x_ = 0;
    float y_ = 0;
};

template <event_type T>
class mouse_wheel_event : public common_event<T>
{
  public:
    constexpr mouse_wheel_event() noexcept = default;

    explicit mouse_wheel_event(SDL_Event const& e) noexcept
        : common_event<T>(e),
          window_id_(static_cast<eggs::sdl::window_id>(e.wheel.windowID)),
          which_(static_cast<mouse_id>(e.wheel.which)),
          x_(e.wheel.x),
          y_(e.wheel.y),
          direction_(static_cast<mouse_wheel_direction>(e.wheel.direction)),
          mouse_x_(e.wheel.mouse_x),
          mouse_y_(e.wheel.mouse_y)
    {
    }

    [[nodiscard]] constexpr auto window_id() const noexcept
        -> eggs::sdl::window_id
    {
        return window_id_;
    }

    [[nodiscard]] constexpr auto which() const noexcept -> mouse_id
    {
        return which_;
    }

    [[nodiscard]] constexpr auto x() const noexcept -> float { return x_; }

    [[nodiscard]] constexpr auto y() const noexcept -> float { return y_; }

    [[nodiscard]] constexpr auto direction() const noexcept
        -> mouse_wheel_direction
    {
        return direction_;
    }

    [[nodiscard]] constexpr auto mouse_x() const noexcept -> float
    {
        return mouse_x_;
    }

    [[nodiscard]] constexpr auto mouse_y() const noexcept -> float
    {
        return mouse_y_;
    }

  private:
    eggs::sdl::window_id window_id_{};
    mouse_id which_{};
    float x_ = 0;
    float y_ = 0;
    mouse_wheel_direction direction_{};
    float mouse_x_ = 0;
    float mouse_y_ = 0;
};

} // namespace detail

// Only the common fields are exposed for the event types that are not listed
// with accessors here. The text of text_editing_event, text_input_event and
// the drop events is not exposed either, SDL owns it only until the next poll
// and the alternatives do not alias the event.
#define EGGS_SDL_EVENT_ALIAS(name, value, family) \
    using name##_event = detail::family##_event<event_type::name>;
EGGS_SDL_EVENT_TYPES(EGGS_SDL_EVENT_ALIAS)
#undef EGGS_SDL_EVENT_ALIAS

// An event type that is not otherwise covered, such as one added by a newer SDL
// release.
class unknown_event
{
  public:
    [[nodiscard]] static constexpr auto matches(event_type value) noexcept
        -> bool
    {
        return !detail::is_known_type(value) && !detail::is_user_type(value);
    }

    constexpr unknown_event() noexcept = default;

    explicit unknown_event(SDL_Event const& e) noexcept
        : type_{e.type},
          timestamp_(detail::to_nanoseconds(e.common.timestamp))
    {
    }

    [[nodiscard]] constexpr auto type() const noexcept -> event_type
    {
        return type_;
    }

    [[nodiscard]] constexpr auto timestamp() const noexcept
        -> std::chrono::nanoseconds
    {
        return timestamp_;
    }

  private:
    event_type type_{};
    std::chrono::nanoseconds timestamp_{};
};

// An event type handed out by register_events. The data pointers are the
// caller's, SDL does not own them.
class user_event
{
  public:
    [[nodiscard]] static constexpr auto matches(event_type value) noexcept
        -> bool
    {
        return detail::is_user_type(value);
    }

    constexpr user_event() noexcept = default;

    explicit user_event(SDL_Event const& e) noexcept
        : type_{e.user.type},
          timestamp_(detail::to_nanoseconds(e.user.timestamp)),
          window_id_(static_cast<eggs::sdl::window_id>(e.user.windowID)),
          code_(e.user.code),
          data1_(e.user.data1),
          data2_(e.user.data2)
    {
    }

    [[nodiscard]] constexpr auto type() const noexcept -> event_type
    {
        return type_;
    }

    [[nodiscard]] constexpr auto timestamp() const noexcept
        -> std::chrono::nanoseconds
    {
        return timestamp_;
    }

    [[nodiscard]] constexpr auto window_id() const noexcept
        -> eggs::sdl::window_id
    {
        return window_id_;
    }

    [[nodiscard]] constexpr auto code() const noexcept -> std::int32_t
    {
        return code_;
    }

    template <typename T = void>
    [[nodiscard]] auto data1() const noexcept -> T*
    {
        return static_cast<T*>(data1_);
    }

    template <typename T = void>
    [[nodiscard]] auto data2() const noexcept -> T*
    {
        return static_cast<T*>(data2_);
    }

  private:
    event_type type_{};
    std::chrono::nanoseconds timestamp_{};
    eggs::sdl::window_id window_id_{};
    std::int32_t code_ = 0;
    void* data1_ = nullptr;
    void* data2_ = nullptr;
};

#ifdef __clang__
#    pragma clang diagnostic pop
#endif

template <typename E>
concept event_alternative = requires(SDL_Event const& raw, event_type type) {
    { E::matches(type) } -> std::same_as<bool>;
    E(raw);
};

// Holds a copy of the SDL event. Not thread-safe.
class event
{
  public:
    event() noexcept = default;

    [[nodiscard]] auto type() const noexcept -> event_type
    {
        return event_type{event_.type};
    }

    template <event_alternative E>
    [[nodiscard]] auto holds_alternative() const noexcept -> bool
    {
        return E::matches(type());
    }

    template <event_alternative E>
    [[nodiscard]] auto get_if() const noexcept -> std::optional<E>
    {
        if (!holds_alternative<E>()) return std::nullopt;
        return E(event_);
    }

    template <event_alternative E>
    [[nodiscard]] auto get() const noexcept -> E
    {
        assert(holds_alternative<E>());
        return E(event_);
    }

    template <typename F>
    auto visit(F&& visitor) const -> decltype(auto)
    {
#ifdef __clang__
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wswitch-enum"
#endif
        switch (type()) {
#define EGGS_SDL_EVENT_VISIT(name, value, family) \
    case event_type::name:                        \
        return static_cast<F&&>(visitor)(name##_event(event_));
            EGGS_SDL_EVENT_TYPES(EGGS_SDL_EVENT_VISIT)
#undef EGGS_SDL_EVENT_VISIT
            default:
                break;
        }
#ifdef __clang__
#    pragma clang diagnostic pop
#endif
        if (user_event::matches(type()))
            return static_cast<F&&>(visitor)(user_event(event_));
        return static_cast<F&&>(visitor)(unknown_event(event_));
    }

    [[nodiscard]] auto sdl() const noexcept -> SDL_Event const&
    {
        return event_;
    }

    [[nodiscard]] auto sdl() noexcept -> SDL_Event& { return event_; }

  private:
    // Zero all of it: {} would only initialize the first member of the union.
    SDL_Event event_ = std::bit_cast<SDL_Event>(
        std::array<unsigned char, sizeof(SDL_Event)>{}
    );
};

// Event functions must be called from the thread that initialized video,
// usually the main thread, except for push, register_events, add_watch and
// set_filter, which are thread-safe.

// Gathers pending input into the queue. poll and wait do it implicitly.
inline void pump() noexcept
{
    SDL_PumpEvents();
}

// false when the poll cycle is over, and then out is unspecified. SDL ends each
// cycle with an internal sentinel, so events pushed after the previous false
// return may need one more poll to be returned.
[[nodiscard]] inline auto poll(event& out) noexcept -> bool
{
    return SDL_PollEvent(&out.sdl());
}

[[nodiscard]] inline auto poll() -> std::optional<event>
{
    event result;
    if (!poll(result)) return std::nullopt;
    return result;
}

[[nodiscard]] inline auto wait(event& out) -> std::expected<void, error>
{
    return detail::check(SDL_WaitEvent(&out.sdl()));
}

// A negative timeout waits indefinitely, a zero timeout does not wait. Values
// that do not fit in 32 bits are clamped. false on timeout or error: SDL cannot
// tell them apart.
[[nodiscard]] inline auto
wait_timeout(event& out, std::chrono::milliseconds timeout) noexcept -> bool
{
    auto const clamped = std::clamp<std::chrono::milliseconds::rep>(
        timeout.count(), std::numeric_limits<std::int32_t>::min(),
        std::numeric_limits<std::int32_t>::max()
    );
    return SDL_WaitEventTimeout(&out.sdl(), static_cast<std::int32_t>(clamped));
}

// SDL copies the event. An empty error message means that the event was
// rejected by the event filter, SDL does not report that as an error. Watchers
// run before the event is queued, on the calling thread.
[[nodiscard]] inline auto push(event const& e) -> std::expected<void, error>
{
    SDL_Event copy = e.sdl();
    return detail::check(SDL_PushEvent(&copy));
}

// The first of count consecutive event types for use with user_event. They
// are never released.
[[nodiscard]] inline auto register_events(int count)
    -> std::expected<event_type, error>
{
    std::uint32_t const first = SDL_RegisterEvents(count);
    if (first == 0) return std::unexpected(error::current());
    return static_cast<event_type>(first);
}

// Removes the watch on destruction. Adopts an SDL event watch that is already
// registered with this callback and userdata, see add_watch.
class event_watch
{
  public:
    event_watch() noexcept = default;

    event_watch(SDL_EventFilter callback, void* userdata) noexcept
        : callback_(callback),
          userdata_(userdata)
    {
    }

    event_watch(event_watch const&) = delete;
    auto operator=(event_watch const&) -> event_watch& = delete;

    event_watch(event_watch&& other) noexcept
        : callback_(std::exchange(other.callback_, nullptr)),
          userdata_(std::exchange(other.userdata_, nullptr))
    {
    }

    auto operator=(event_watch&& other) noexcept -> event_watch&
    {
        if (this != &other) {
            remove();
            callback_ = std::exchange(other.callback_, nullptr);
            userdata_ = std::exchange(other.userdata_, nullptr);
        }
        return *this;
    }

    ~event_watch() { remove(); }

  private:
    void remove() noexcept
    {
        if (callback_ != nullptr) SDL_RemoveEventWatch(callback_, userdata_);
        callback_ = nullptr;
        userdata_ = nullptr;
    }

    SDL_EventFilter callback_ = nullptr;
    void* userdata_ = nullptr;
};

// Clears the filter on destruction, unless it was replaced in the meantime.
// Adopts an SDL event filter that is already set, see set_filter.
class event_filter
{
  public:
    event_filter() noexcept = default;

    event_filter(SDL_EventFilter callback, void* userdata) noexcept
        : callback_(callback),
          userdata_(userdata)
    {
    }

    event_filter(event_filter const&) = delete;
    auto operator=(event_filter const&) -> event_filter& = delete;

    event_filter(event_filter&& other) noexcept
        : callback_(std::exchange(other.callback_, nullptr)),
          userdata_(std::exchange(other.userdata_, nullptr))
    {
    }

    auto operator=(event_filter&& other) noexcept -> event_filter&
    {
        if (this != &other) {
            clear();
            callback_ = std::exchange(other.callback_, nullptr);
            userdata_ = std::exchange(other.userdata_, nullptr);
        }
        return *this;
    }

    ~event_filter() { clear(); }

  private:
    void clear() noexcept
    {
        SDL_EventFilter current = nullptr;
        void* current_userdata = nullptr;
        if (callback_ != nullptr &&
            SDL_GetEventFilter(&current, &current_userdata) &&
            current == callback_ && current_userdata == userdata_)
            SDL_SetEventFilter(nullptr, nullptr);
        callback_ = nullptr;
        userdata_ = nullptr;
    }

    SDL_EventFilter callback_ = nullptr;
    void* userdata_ = nullptr;
};

namespace detail {

[[nodiscard]] inline auto make_event(SDL_Event const& raw) noexcept -> event
{
    event result;
    result.sdl() = raw;
    return result;
}

template <typename F>
[[nodiscard]] auto callback_userdata(F& callback) noexcept -> void*
{
    return const_cast<void*>(
        static_cast<void const volatile*>(std::addressof(callback))
    );
}

template <typename F>
struct callback_thunk
{
    static bool SDLCALL watch(void* userdata, SDL_Event* raw) noexcept
    {
        (*static_cast<F*>(userdata))(make_event(*raw));
        return true;
    }

    static bool SDLCALL filter(void* userdata, SDL_Event* raw) noexcept
    {
        return (*static_cast<F*>(userdata))(make_event(*raw));
    }
};

} // namespace detail

// The callback, void(event const&), is called for every event that is pushed
// or generated, from whichever thread did it, and must not throw. It is not
// copied: the caller keeps it alive for as long as the watch is registered.
template <typename F>
[[nodiscard]] auto add_watch(F& callback) -> std::expected<event_watch, error>
{
    auto const thunk = &detail::callback_thunk<F>::watch;
    void* const userdata = detail::callback_userdata(callback);
    if (!SDL_AddEventWatch(thunk, userdata))
        return std::unexpected(error::current());
    return event_watch(thunk, userdata);
}

// The callback, bool(event const&), returns true to keep the event, and is
// called from whichever thread adds the event. It must not throw. It is not
// copied: the caller keeps it alive for as long as the filter is set. SDL has
// a single filter: this replaces the previous one, and drops the pending
// events that the callback rejects.
template <typename F>
[[nodiscard]] auto set_filter(F& callback) -> event_filter
{
    auto const thunk = &detail::callback_thunk<F>::filter;
    void* const userdata = detail::callback_userdata(callback);
    SDL_SetEventFilter(thunk, userdata);
    return event_filter(thunk, userdata);
}

} // namespace eggs::sdl
