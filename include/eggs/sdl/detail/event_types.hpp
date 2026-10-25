// Eggs.SDL
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#pragma once

// One row per SDL event type: X(name, SDL value, family). The alternative is
// named <name>_event and is an instance of detail::<family>_event, which gives
// it its accessors. Adding an SDL event type is adding a row.
#define EGGS_SDL_EVENT_TYPES(X)                                               \
    X(quit, SDL_EVENT_QUIT, common)                                           \
    X(terminating, SDL_EVENT_TERMINATING, common)                             \
    X(low_memory, SDL_EVENT_LOW_MEMORY, common)                               \
    X(will_enter_background, SDL_EVENT_WILL_ENTER_BACKGROUND, common)         \
    X(did_enter_background, SDL_EVENT_DID_ENTER_BACKGROUND, common)           \
    X(will_enter_foreground, SDL_EVENT_WILL_ENTER_FOREGROUND, common)         \
    X(did_enter_foreground, SDL_EVENT_DID_ENTER_FOREGROUND, common)           \
    X(locale_changed, SDL_EVENT_LOCALE_CHANGED, common)                       \
    X(system_theme_changed, SDL_EVENT_SYSTEM_THEME_CHANGED, common)           \
    X(display_orientation, SDL_EVENT_DISPLAY_ORIENTATION, common)             \
    X(display_added, SDL_EVENT_DISPLAY_ADDED, common)                         \
    X(display_removed, SDL_EVENT_DISPLAY_REMOVED, common)                     \
    X(display_moved, SDL_EVENT_DISPLAY_MOVED, common)                         \
    X(display_desktop_mode_changed, SDL_EVENT_DISPLAY_DESKTOP_MODE_CHANGED,   \
      common)                                                                 \
    X(display_current_mode_changed, SDL_EVENT_DISPLAY_CURRENT_MODE_CHANGED,   \
      common)                                                                 \
    X(display_content_scale_changed, SDL_EVENT_DISPLAY_CONTENT_SCALE_CHANGED, \
      common)                                                                 \
    X(window_shown, SDL_EVENT_WINDOW_SHOWN, window)                           \
    X(window_hidden, SDL_EVENT_WINDOW_HIDDEN, window)                         \
    X(window_exposed, SDL_EVENT_WINDOW_EXPOSED, window)                       \
    X(window_moved, SDL_EVENT_WINDOW_MOVED, window_position)                  \
    X(window_resized, SDL_EVENT_WINDOW_RESIZED, window_size)                  \
    X(window_pixel_size_changed, SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED,         \
      window_size)                                                            \
    X(window_metal_view_resized, SDL_EVENT_WINDOW_METAL_VIEW_RESIZED, window) \
    X(window_minimized, SDL_EVENT_WINDOW_MINIMIZED, window)                   \
    X(window_maximized, SDL_EVENT_WINDOW_MAXIMIZED, window)                   \
    X(window_restored, SDL_EVENT_WINDOW_RESTORED, window)                     \
    X(window_mouse_enter, SDL_EVENT_WINDOW_MOUSE_ENTER, window)               \
    X(window_mouse_leave, SDL_EVENT_WINDOW_MOUSE_LEAVE, window)               \
    X(window_focus_gained, SDL_EVENT_WINDOW_FOCUS_GAINED, window)             \
    X(window_focus_lost, SDL_EVENT_WINDOW_FOCUS_LOST, window)                 \
    X(window_close_requested, SDL_EVENT_WINDOW_CLOSE_REQUESTED, window)       \
    X(window_hit_test, SDL_EVENT_WINDOW_HIT_TEST, window)                     \
    X(window_iccprof_changed, SDL_EVENT_WINDOW_ICCPROF_CHANGED, window)       \
    X(window_display_changed, SDL_EVENT_WINDOW_DISPLAY_CHANGED,               \
      window_display)                                                         \
    X(window_display_scale_changed, SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED,   \
      window)                                                                 \
    X(window_safe_area_changed, SDL_EVENT_WINDOW_SAFE_AREA_CHANGED, window)   \
    X(window_occluded, SDL_EVENT_WINDOW_OCCLUDED, window)                     \
    X(window_enter_fullscreen, SDL_EVENT_WINDOW_ENTER_FULLSCREEN, window)     \
    X(window_leave_fullscreen, SDL_EVENT_WINDOW_LEAVE_FULLSCREEN, window)     \
    X(window_destroyed, SDL_EVENT_WINDOW_DESTROYED, window)                   \
    X(window_hdr_state_changed, SDL_EVENT_WINDOW_HDR_STATE_CHANGED, window)   \
    X(key_down, SDL_EVENT_KEY_DOWN, key)                                      \
    X(key_up, SDL_EVENT_KEY_UP, key)                                          \
    X(text_editing, SDL_EVENT_TEXT_EDITING, common)                           \
    X(text_input, SDL_EVENT_TEXT_INPUT, common)                               \
    X(keymap_changed, SDL_EVENT_KEYMAP_CHANGED, common)                       \
    X(keyboard_added, SDL_EVENT_KEYBOARD_ADDED, keyboard_device)              \
    X(keyboard_removed, SDL_EVENT_KEYBOARD_REMOVED, keyboard_device)          \
    X(text_editing_candidates, SDL_EVENT_TEXT_EDITING_CANDIDATES, common)     \
    X(mouse_motion, SDL_EVENT_MOUSE_MOTION, mouse_motion)                     \
    X(mouse_button_down, SDL_EVENT_MOUSE_BUTTON_DOWN, mouse_button)           \
    X(mouse_button_up, SDL_EVENT_MOUSE_BUTTON_UP, mouse_button)               \
    X(mouse_wheel, SDL_EVENT_MOUSE_WHEEL, mouse_wheel)                        \
    X(mouse_added, SDL_EVENT_MOUSE_ADDED, mouse_device)                       \
    X(mouse_removed, SDL_EVENT_MOUSE_REMOVED, mouse_device)                   \
    X(joystick_axis_motion, SDL_EVENT_JOYSTICK_AXIS_MOTION, common)           \
    X(joystick_ball_motion, SDL_EVENT_JOYSTICK_BALL_MOTION, common)           \
    X(joystick_hat_motion, SDL_EVENT_JOYSTICK_HAT_MOTION, common)             \
    X(joystick_button_down, SDL_EVENT_JOYSTICK_BUTTON_DOWN, common)           \
    X(joystick_button_up, SDL_EVENT_JOYSTICK_BUTTON_UP, common)               \
    X(joystick_added, SDL_EVENT_JOYSTICK_ADDED, common)                       \
    X(joystick_removed, SDL_EVENT_JOYSTICK_REMOVED, common)                   \
    X(joystick_battery_updated, SDL_EVENT_JOYSTICK_BATTERY_UPDATED, common)   \
    X(joystick_update_complete, SDL_EVENT_JOYSTICK_UPDATE_COMPLETE, common)   \
    X(gamepad_axis_motion, SDL_EVENT_GAMEPAD_AXIS_MOTION, common)             \
    X(gamepad_button_down, SDL_EVENT_GAMEPAD_BUTTON_DOWN, common)             \
    X(gamepad_button_up, SDL_EVENT_GAMEPAD_BUTTON_UP, common)                 \
    X(gamepad_added, SDL_EVENT_GAMEPAD_ADDED, common)                         \
    X(gamepad_removed, SDL_EVENT_GAMEPAD_REMOVED, common)                     \
    X(gamepad_remapped, SDL_EVENT_GAMEPAD_REMAPPED, common)                   \
    X(gamepad_touchpad_down, SDL_EVENT_GAMEPAD_TOUCHPAD_DOWN, common)         \
    X(gamepad_touchpad_motion, SDL_EVENT_GAMEPAD_TOUCHPAD_MOTION, common)     \
    X(gamepad_touchpad_up, SDL_EVENT_GAMEPAD_TOUCHPAD_UP, common)             \
    X(gamepad_sensor_update, SDL_EVENT_GAMEPAD_SENSOR_UPDATE, common)         \
    X(gamepad_update_complete, SDL_EVENT_GAMEPAD_UPDATE_COMPLETE, common)     \
    X(gamepad_steam_handle_updated, SDL_EVENT_GAMEPAD_STEAM_HANDLE_UPDATED,   \
      common)                                                                 \
    X(finger_down, SDL_EVENT_FINGER_DOWN, common)                             \
    X(finger_up, SDL_EVENT_FINGER_UP, common)                                 \
    X(finger_motion, SDL_EVENT_FINGER_MOTION, common)                         \
    X(finger_canceled, SDL_EVENT_FINGER_CANCELED, common)                     \
    X(clipboard_update, SDL_EVENT_CLIPBOARD_UPDATE, common)                   \
    X(drop_file, SDL_EVENT_DROP_FILE, common)                                 \
    X(drop_text, SDL_EVENT_DROP_TEXT, common)                                 \
    X(drop_begin, SDL_EVENT_DROP_BEGIN, common)                               \
    X(drop_complete, SDL_EVENT_DROP_COMPLETE, common)                         \
    X(drop_position, SDL_EVENT_DROP_POSITION, common)                         \
    X(audio_device_added, SDL_EVENT_AUDIO_DEVICE_ADDED, common)               \
    X(audio_device_removed, SDL_EVENT_AUDIO_DEVICE_REMOVED, common)           \
    X(audio_device_format_changed, SDL_EVENT_AUDIO_DEVICE_FORMAT_CHANGED,     \
      common)                                                                 \
    X(sensor_update, SDL_EVENT_SENSOR_UPDATE, common)                         \
    X(pen_proximity_in, SDL_EVENT_PEN_PROXIMITY_IN, common)                   \
    X(pen_proximity_out, SDL_EVENT_PEN_PROXIMITY_OUT, common)                 \
    X(pen_down, SDL_EVENT_PEN_DOWN, common)                                   \
    X(pen_up, SDL_EVENT_PEN_UP, common)                                       \
    X(pen_button_down, SDL_EVENT_PEN_BUTTON_DOWN, common)                     \
    X(pen_button_up, SDL_EVENT_PEN_BUTTON_UP, common)                         \
    X(pen_motion, SDL_EVENT_PEN_MOTION, common)                               \
    X(pen_axis, SDL_EVENT_PEN_AXIS, common)                                   \
    X(camera_device_added, SDL_EVENT_CAMERA_DEVICE_ADDED, common)             \
    X(camera_device_removed, SDL_EVENT_CAMERA_DEVICE_REMOVED, common)         \
    X(camera_device_approved, SDL_EVENT_CAMERA_DEVICE_APPROVED, common)       \
    X(camera_device_denied, SDL_EVENT_CAMERA_DEVICE_DENIED, common)           \
    X(render_targets_reset, SDL_EVENT_RENDER_TARGETS_RESET, common)           \
    X(render_device_reset, SDL_EVENT_RENDER_DEVICE_RESET, common)             \
    X(render_device_lost, SDL_EVENT_RENDER_DEVICE_LOST, common)
