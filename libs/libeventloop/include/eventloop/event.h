#pragma once

#include <eventloop/event_gen.h>
#include <liim/function.h>
#include <liim/string.h>
#include <liim/string_view.h>

namespace App {
class Event {
    APP_EVENT_HEADER(App, Event)

public:
    explicit Event(StringView name) : m_name(name) {}
    virtual ~Event() {}

    StringView name() const { return m_name; }

private:
    StringView m_name;
};

namespace MouseButton {
    enum {
        None = 0,
        Left = 1,
        Right = 2,
        Middle = 4,
    };
}

namespace KeyModifier {
    enum {
        Shift = 1,
        Alt = 2,
        Control = 4,
        Meta = 8,
    };
}

enum class Key {
    None = 0,
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,
    _1,
    _2,
    _3,
    _4,
    _5,
    _6,
    _7,
    _8,
    _9,
    _0,
    Enter,
    Escape,
    Backspace,
    Tab,
    Space,
    Minus,
    Equals,
    LeftBracket,
    RightBracket,
    Backslash,
    NonUS_Pound,
    SemiColon,
    Quote,
    Backtick,
    Comma,
    Period,
    Slash,
    CapsLock,
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
    PrintScreen,
    ScrollLock,
    Pause,
    Insert,
    Home,
    PageUp,
    Delete,
    End,
    PageDown,
    RightArrow,
    LeftArrow,
    DownArrow,
    UpArrow,
    NumLock,
    Numpad_Slash,
    Numpad_Star,
    Numpad_Minus,
    Numpad_Plus,
    Numpad_Enter,
    Numpad_1,
    Numpad_2,
    Numpad_3,
    Numpad_4,
    Numpad_5,
    Numpad_6,
    Numpad_7,
    Numpad_8,
    Numpad_9,
    Numpad_0,
    Numpad_Period,
    NonUS_Backslash,
    Application,
    Power,
    Numpad_Equals,
    F13,
    F14,
    F15,
    F16,
    F17,
    F18,
    F19,
    F20,
    F21,
    F22,
    F23,
    F24,
    LeftControl,
    LeftShift,
    LeftAlt,
    LeftMeta,
    RightControl,
    RightShift,
    RightAlt,
    RightMeta,
};
}

// clang-format off
APP_EVENT_PARENT(App, WindowEvent, Event, ((StringView, name)), (), ())
APP_EVENT(App, WindowCloseEvent, WindowEvent, (), (), ())
APP_EVENT(App, WindowForceRedrawEvent, WindowEvent, (), (), ())
APP_EVENT(App, WindowDidResizeEvent, WindowEvent, (), (), ())
APP_EVENT(App, WindowStateEvent, WindowEvent, (), ((bool, active)), ())

APP_EVENT(App, ResizeEvent, Event, (), (), ())
APP_EVENT(App, FocusedEvent, Event, (), (), ())
APP_EVENT(App, UnfocusedEvent, Event, (), (), ())
APP_EVENT(App, LeaveEvent, Event, (), (), ())
APP_EVENT(App, EnterEvent, Event, (), (), ())

#define __APP_KEY_EVENT_FIELDS ((Key, key), (int, modifiers), (bool, generates_text))

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

#define __APP_MOUSE_EVENT_FIELDS ((int, buttons_down), (int, x), (int, y), (int, z), (int, button), (int, modifiers))

APP_EVENT_PARENT_REQUIRES_HANDLING(App, MouseEvent, Event, ((StringView, name)), __APP_MOUSE_EVENT_FIELDS, (
    (bool left_button() const { return m_button == MouseButton::Left; }),
    (bool right_button() const { return m_button == MouseButton::Right; }),
    (bool middle_button() const { return m_button == MouseButton::Middle; }),

    (bool mouse_down_any() const { return name() == "App::MouseDownEvent" || name() == "App::MouseDoubleEvent" || name() == "App::MouseTripleEvent"; }),

    (bool mouse_down() const { return name() == "App::MouseDownEvent"; }),
    (bool mouse_double() const { return name() == "App::MouseDoubleEvent"; }),
    (bool mouse_triple() const { return name() == "App::MouseTripleEvent"; }),
    (bool mouse_up() const { return name() == "App::MouseUpEvent"; }),
    (bool mouse_move() const { return name() == "App::MouseMoveEvent"; }),
    (bool mouse_scroll() const { return name() == "App::MouseScrollEvent"; }),

    (int control_down() const { return m_modifiers & KeyModifier::Control; }),
    (int shift_down() const { return m_modifiers & KeyModifier::Shift; }),
    (int alt_down() const { return m_modifiers & KeyModifier::Alt; }),
    (int meta_down() const { return m_modifiers & KeyModifier::Meta; })
))

APP_EVENT_REQUIRES_HANDLING(App, MouseDownEvent, MouseEvent, __APP_MOUSE_EVENT_FIELDS, (), ())
APP_EVENT_REQUIRES_HANDLING(App, MouseDoubleEvent, MouseEvent, __APP_MOUSE_EVENT_FIELDS, (), ())
APP_EVENT_REQUIRES_HANDLING(App, MouseTripleEvent, MouseEvent, __APP_MOUSE_EVENT_FIELDS, (), ())
APP_EVENT_REQUIRES_HANDLING(App, MouseMoveEvent, MouseEvent, __APP_MOUSE_EVENT_FIELDS, (), ())
APP_EVENT_REQUIRES_HANDLING(App, MouseUpEvent, MouseEvent, __APP_MOUSE_EVENT_FIELDS, (), ())
APP_EVENT_REQUIRES_HANDLING(App, MouseScrollEvent, MouseEvent, __APP_MOUSE_EVENT_FIELDS, (), ())

APP_EVENT(App, CallbackEvent, Event, (), ((Function<void()>, callback)), (
    void invoke() { m_callback(); }
))

APP_EVENT(App, TimerEvent, Event, (), ((int, times_expired)), ())

APP_EVENT(App, ThemeChangeEvent, Event, (), (), ())
// clang-format on
