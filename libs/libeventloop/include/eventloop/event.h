#pragma once

#include <liim/function.h>
#include <liim/string.h>
#include <liim/string_view.h>

namespace App {
#define APP_EVENT_REQUIRES_HANDLING(name) APP_EVENT_IMPL(name, true)

#define APP_EVENT(name) APP_EVENT_IMPL(name, false)

#define APP_EVENT_IMPL(name, requires_handling)                                          \
public:                                                                                  \
    static constexpr StringView static_event_name() { return "" #name; }                 \
    static constexpr bool static_event_requires_handling() { return requires_handling; } \
                                                                                         \
private:

class Event {
    APP_EVENT(Event)

public:
    explicit Event(StringView name) : m_name(name) {}
    virtual ~Event() {}

    StringView name() const { return m_name; }

private:
    StringView m_name;
};

class WindowEvent : public Event {
    APP_EVENT(WindowEvent)

public:
    explicit WindowEvent(StringView name) : Event(name) {}
};

class WindowCloseEvent : public WindowEvent {
    APP_EVENT(WindowCloseEvent)

public:
    WindowCloseEvent() : WindowEvent(static_event_name()) {}
};

class WindowForceRedrawEvent : public WindowEvent {
    APP_EVENT(WindowForceRedrawEvent)

public:
    WindowForceRedrawEvent() : WindowEvent(static_event_name()) {}
};

class WindowDidResizeEvent : public WindowEvent {
    APP_EVENT(WindowDidResizeEvent)

public:
    WindowDidResizeEvent() : WindowEvent(static_event_name()) {}
};

class WindowStateEvent final : public WindowEvent {
    APP_EVENT(WindowStateEvent)

public:
    explicit WindowStateEvent(bool active) : WindowEvent(static_event_name()), m_active(active) {}

    bool active() const { return m_active; }
    void set_active(bool b) { m_active = b; }

private:
    bool m_active { false };
};

class ResizeEvent final : public Event {
    APP_EVENT(ResizeEvent)

public:
    ResizeEvent() : Event(static_event_name()) {}
};

class FocusedEvent final : public Event {
    APP_EVENT(FocusedEvent)

public:
    FocusedEvent() : Event(static_event_name()) {}
};

class UnfocusedEvent final : public Event {
    APP_EVENT(UnfocusedEvent)

public:
    UnfocusedEvent() : Event(static_event_name()) {}
};

class LeaveEvent final : public Event {
    APP_EVENT(LeaveEvent)

public:
    LeaveEvent() : Event(static_event_name()) {}
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

class KeyEvent : public Event {
    APP_EVENT_REQUIRES_HANDLING(KeyEvent)

public:
    KeyEvent(StringView name, Key key, int modifiers, bool generates_text)
        : Event(name), m_key(key), m_modifiers(modifiers), m_generates_text(generates_text) {}

    Key key() const { return m_key; }
    int modifiers() const { return m_modifiers; }

    bool key_down() const { return name() == "KeyDownEvent"; }
    bool key_up() const { return name() == "KeyUpEvent"; }

    bool shift_down() const { return !!(m_modifiers & KeyModifier::Shift); }
    bool alt_down() const { return !!(m_modifiers & KeyModifier::Alt); }
    bool control_down() const { return !!(m_modifiers & KeyModifier::Control); }
    bool meta_down() const { return !!(m_modifiers & KeyModifier::Meta); }

    bool generates_text() const { return m_generates_text; }

private:
    Key m_key { Key::None };
    int m_modifiers { 0 };
    bool m_generates_text { false };
};

class KeyDownEvent final : public KeyEvent {
    APP_EVENT_REQUIRES_HANDLING(KeyDownEvent)

public:
    KeyDownEvent(Key key, int modifiers, bool generates_text) : KeyEvent(static_event_name(), key, modifiers, generates_text) {}
};

class KeyUpEvent final : public KeyEvent {
    APP_EVENT_REQUIRES_HANDLING(KeyUpEvent)

public:
    KeyUpEvent(Key key, int modifiers, bool generates_text) : KeyEvent(static_event_name(), key, modifiers, generates_text) {}
};

class TextEvent final : public Event {
    APP_EVENT_REQUIRES_HANDLING(TextEvent)

public:
    explicit TextEvent(String text) : Event(static_event_name()), m_text(move(text)) {}

    const String& text() const { return m_text; }
    void set_text(String text) { m_text = move(text); }

private:
    String m_text;
};

class MouseEvent : public Event {
    APP_EVENT_REQUIRES_HANDLING(MouseEvent)

public:
    MouseEvent(StringView name, int buttons_down, int x, int y, int z, int button, int modifiers)
        : Event(name), m_x(x), m_y(y), m_z(z), m_buttons_down(buttons_down), m_button(button), m_modifiers(modifiers) {}

    int x() const { return m_x; }
    int y() const { return m_y; }
    int z() const { return m_z; }
    int button() const { return m_button; }

    void set_x(int x) { m_x = x; }
    void set_y(int y) { m_y = y; }

    int buttons_down() const { return m_buttons_down; };

    bool left_button() const { return m_button == MouseButton::Left; }
    bool right_button() const { return m_button == MouseButton::Right; }
    bool middle_button() const { return m_button == MouseButton::Middle; }

    bool mouse_down_any() const { return name() == "MouseDownEvent" || name() == "MouseDoubleEvent" || name() == "MouseTripleEvent"; }

    bool mouse_down() const { return name() == "MouseDownEvent"; }
    bool mouse_double() const { return name() == "MouseDoubleEvent"; }
    bool mouse_triple() const { return name() == "MouseTripleEvent"; }
    bool mouse_up() const { return name() == "MouseUpEvent"; }
    bool mouse_move() const { return name() == "MouseMoveEvent"; }
    bool mouse_scroll() const { return name() == "MouseScrollEvent"; }

    int modifiers() const { return m_modifiers; }
    int control_down() const { return m_modifiers & KeyModifier::Control; }
    int shift_down() const { return m_modifiers & KeyModifier::Shift; }
    int alt_down() const { return m_modifiers & KeyModifier::Alt; }
    int meta_down() const { return m_modifiers & KeyModifier::Meta; }

private:
    int m_x { 0 };
    int m_y { 0 };
    int m_z { 0 };
    int m_buttons_down { 0 };
    int m_button { 0 };
    int m_modifiers { 0 };
};

class MouseDownEvent final : public MouseEvent {
    APP_EVENT_REQUIRES_HANDLING(MouseDownEvent)

public:
    MouseDownEvent(int buttons_down, int x, int y, int z, int button, int modifiers)
        : MouseEvent(static_event_name(), buttons_down, x, y, z, button, modifiers) {}
};

class MouseDoubleEvent final : public MouseEvent {
    APP_EVENT_REQUIRES_HANDLING(MouseDoubleEvent)

public:
    MouseDoubleEvent(int buttons_down, int x, int y, int z, int button, int modifiers)
        : MouseEvent(static_event_name(), buttons_down, x, y, z, button, modifiers) {}
};

class MouseTripleEvent final : public MouseEvent {
    APP_EVENT_REQUIRES_HANDLING(MouseTripleEvent)

public:
    MouseTripleEvent(int buttons_down, int x, int y, int z, int button, int modifiers)
        : MouseEvent(static_event_name(), buttons_down, x, y, z, button, modifiers) {}
};

class MouseMoveEvent final : public MouseEvent {
    APP_EVENT_REQUIRES_HANDLING(MouseMoveEvent)

public:
    MouseMoveEvent(int buttons_down, int x, int y, int z, int button, int modifiers)
        : MouseEvent(static_event_name(), buttons_down, x, y, z, button, modifiers) {}
};

class MouseUpEvent final : public MouseEvent {
    APP_EVENT_REQUIRES_HANDLING(MouseUpEvent)

public:
    MouseUpEvent(int buttons_down, int x, int y, int z, int button, int modifiers)
        : MouseEvent(static_event_name(), buttons_down, x, y, z, button, modifiers) {}
};

class MouseScrollEvent final : public MouseEvent {
    APP_EVENT_REQUIRES_HANDLING(MouseScrollEvent)

public:
    MouseScrollEvent(int buttons_down, int x, int y, int z, int button, int modifiers)
        : MouseEvent(static_event_name(), buttons_down, x, y, z, button, modifiers) {}
};

class CallbackEvent final : public Event {
    APP_EVENT(CallbackEvent)

public:
    explicit CallbackEvent(Function<void()> callback) : Event(static_event_name()), m_callback(move(callback)) {}

    void invoke() { m_callback(); }

private:
    Function<void()> m_callback;
};

class TimerEvent final : public Event {
    APP_EVENT(TimerEvent)

public:
    explicit TimerEvent(int times_expired) : Event(static_event_name()), m_times_expired(times_expired) {}

    int times_expired() const { return m_times_expired; }

private:
    int m_times_expired;
};

class ThemeChangeEvent final : public Event {
    APP_EVENT(ThemeChangeEvent)

public:
    ThemeChangeEvent() : Event(static_event_name()) {}
};
}
