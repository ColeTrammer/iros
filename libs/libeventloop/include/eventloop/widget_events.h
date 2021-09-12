#pragma once

#include <eventloop/event.h>
#include <eventloop/input_constants.h>
#include <liim/string.h>
#include <liim/string_view.h>

APP_EVENT(App, ResizeEvent, Event, (), (), ())
APP_EVENT(App, FocusedEvent, Event, (), (), ())
APP_EVENT(App, UnfocusedEvent, Event, (), (), ())
APP_EVENT(App, LeaveEvent, Event, (), (), ())
APP_EVENT(App, EnterEvent, Event, (), (), ())

#define __APP_KEY_EVENT_FIELDS ((Key, key), (int, modifiers), (bool, generates_text), (bool, is_multi))

// clang-format off
APP_EVENT_PARENT_REQUIRES_HANDLING(App, KeyEvent, Event, ((StringView, name)), __APP_KEY_EVENT_FIELDS, (
    (bool key_down() const { return name() == "App::KeyDownEvent"; }),
    (bool key_up() const { return name() == "App::KeyUpEvent"; }),

    (bool shift_down() const { return !!(m_modifiers & KeyModifier::Shift); }),
    (bool alt_down() const { return !!(m_modifiers & KeyModifier::Alt); }),
    (bool control_down() const { return !!(m_modifiers & KeyModifier::Control); }),
    (bool meta_down() const { return !!(m_modifiers & KeyModifier::Meta); })
))

APP_EVENT_REQUIRES_HANDLING(App, KeyDownEvent, KeyEvent, __APP_KEY_EVENT_FIELDS, (), ())
APP_EVENT_REQUIRES_HANDLING(App, KeyUpEvent, KeyEvent, __APP_KEY_EVENT_FIELDS, (), ())

APP_EVENT_REQUIRES_HANDLING(App, TextEvent, Event, (), ((String, text)), ())

#define __APP_MOUSE_EVENT_FIELDS ((int, buttons_down), (int, x), (int, y), (int, z), (int, button), (int, count), (int, modifiers))

APP_EVENT_PARENT_REQUIRES_HANDLING(App, MouseEvent, Event, ((StringView, name)), __APP_MOUSE_EVENT_FIELDS, (
    (int cyclic_count(int modulo) const { return 1 + ((m_count - 1) % modulo); }),

    (bool left_button() const { return m_button == MouseButton::Left; }),
    (bool right_button() const { return m_button == MouseButton::Right; }),
    (bool middle_button() const { return m_button == MouseButton::Middle; }),

    (bool mouse_down() const { return name() == "App::MouseDownEvent"; }),
    (bool mouse_up() const { return name() == "App::MouseUpEvent"; }),
    (bool mouse_move() const { return name() == "App::MouseMoveEvent"; }),
    (bool mouse_scroll() const { return name() == "App::MouseScrollEvent"; }),

    (int control_down() const { return m_modifiers & KeyModifier::Control; }),
    (int shift_down() const { return m_modifiers & KeyModifier::Shift; }),
    (int alt_down() const { return m_modifiers & KeyModifier::Alt; }),
    (int meta_down() const { return m_modifiers & KeyModifier::Meta; })
))
// clang-format on

APP_EVENT_REQUIRES_HANDLING(App, MouseDownEvent, MouseEvent, __APP_MOUSE_EVENT_FIELDS, (), ())
APP_EVENT_REQUIRES_HANDLING(App, MouseMoveEvent, MouseEvent, __APP_MOUSE_EVENT_FIELDS, (), ())
APP_EVENT_REQUIRES_HANDLING(App, MouseUpEvent, MouseEvent, __APP_MOUSE_EVENT_FIELDS, (), ())
APP_EVENT_REQUIRES_HANDLING(App, MouseScrollEvent, MouseEvent, __APP_MOUSE_EVENT_FIELDS, (), ())

APP_EVENT(App, ThemeChangeEvent, Event, (), (), ())
