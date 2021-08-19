#pragma once

#include <liim/function.h>
#include <liim/string.h>

namespace App {
enum class EventType {
    Invalid = 0,
    Text,
    KeyDown,
    KeyUp,
    MouseDown,
    MouseDouble,
    MouseTriple,
    MouseMove,
    MouseUp,
    MouseScroll,
    WindowClose,
    WindowForceRedraw,
    WindowDidResize,
    WindowState,
    Resize,
    Focused,
    Unfocused,
    Leave,
    Callback,
    Timer,
    ThemeChange,
};

class Event {
public:
    explicit Event(EventType type) : m_type(type) {};
    virtual ~Event() {}

    EventType type() const { return m_type; }

private:
    EventType m_type { EventType::Invalid };
};

class WindowEvent : public Event {
public:
    explicit WindowEvent(EventType type) : Event(type), m_type(type) {}

private:
    EventType m_type { EventType::Invalid };
};

class WindowCloseEvent : public WindowEvent {
public:
    static constexpr EventType static_type() { return EventType::WindowClose; }

    WindowCloseEvent() : WindowEvent(static_type()) {}
};

class WindowForceRedrawEvent : public WindowEvent {
public:
    static constexpr EventType static_type() { return EventType::WindowForceRedraw; }

    WindowForceRedrawEvent() : WindowEvent(static_type()) {}
};

class WindowDidResizeEvent : public WindowEvent {
public:
    static constexpr EventType static_type() { return EventType::WindowDidResize; }

    WindowDidResizeEvent() : WindowEvent(static_type()) {}
};

class WindowStateEvent final : public WindowEvent {
public:
    static constexpr EventType static_type() { return EventType::WindowState; }

    explicit WindowStateEvent(bool active) : WindowEvent(static_type()), m_active(active) {}

    bool active() const { return m_active; }
    void set_active(bool b) { m_active = b; }

private:
    bool m_active { false };
};

class ResizeEvent final : public Event {
public:
    static constexpr EventType static_type() { return EventType::Resize; }

    ResizeEvent() : Event(static_type()) {}
};

class FocusedEvent final : public Event {
public:
    static constexpr EventType static_type() { return EventType::Focused; }

    FocusedEvent() : Event(static_type()) {}
};

class UnfocusedEvent final : public Event {
public:
    static constexpr EventType static_type() { return EventType::Unfocused; }

    UnfocusedEvent() : Event(static_type()) {}
};

class LeaveEvent final : public Event {
public:
    static constexpr EventType static_type() { return EventType::Leave; }

    LeaveEvent() : Event(static_type()) {}
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
public:
    KeyEvent(EventType type, Key key, int modifiers, bool generates_text)
        : Event(type), m_key(key), m_modifiers(modifiers), m_generates_text(generates_text) {}

    Key key() const { return m_key; }
    int modifiers() const { return m_modifiers; }

    bool key_down() const { return type() == EventType::KeyDown; }
    bool key_up() const { return type() == EventType::KeyUp; }

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
public:
    static constexpr EventType static_type() { return EventType::KeyDown; }

    KeyDownEvent(Key key, int modifiers, bool generates_text) : KeyEvent(static_type(), key, modifiers, generates_text) {}
};

class KeyUpEvent final : public KeyEvent {
public:
    static constexpr EventType static_type() { return EventType::KeyUp; }

    KeyUpEvent(Key key, int modifiers, bool generates_text) : KeyEvent(static_type(), key, modifiers, generates_text) {}
};

class TextEvent final : public Event {
public:
    static constexpr EventType static_type() { return EventType::Text; }

    explicit TextEvent(String text) : Event(static_type()), m_text(move(text)) {}

    const String& text() const { return m_text; }
    void set_text(String text) { m_text = move(text); }

private:
    String m_text;
};

class MouseEvent : public Event {
public:
    MouseEvent(EventType type, int buttons_down, int x, int y, int z, int button, int modifiers)
        : Event(type), m_x(x), m_y(y), m_z(z), m_buttons_down(buttons_down), m_button(button), m_modifiers(modifiers) {}

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

    bool mouse_down_any() const {
        return type() == EventType::MouseDown || type() == EventType::MouseDouble || type() == EventType::MouseTriple;
    }

    bool mouse_down() const { return type() == EventType::MouseDown; }
    bool mouse_double() const { return type() == EventType::MouseDouble; }
    bool mouse_triple() const { return type() == EventType::MouseTriple; }
    bool mouse_up() const { return type() == EventType::MouseUp; }
    bool mouse_move() const { return type() == EventType::MouseMove; }
    bool mouse_scroll() const { return type() == EventType::MouseScroll; }

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
public:
    static constexpr EventType static_type() { return EventType::MouseDown; }

    MouseDownEvent(int buttons_down, int x, int y, int z, int button, int modifiers)
        : MouseEvent(static_type(), buttons_down, x, y, z, button, modifiers) {}
};

class MouseDoubleEvent final : public MouseEvent {
public:
    static constexpr EventType static_type() { return EventType::MouseDouble; }

    MouseDoubleEvent(int buttons_down, int x, int y, int z, int button, int modifiers)
        : MouseEvent(static_type(), buttons_down, x, y, z, button, modifiers) {}
};

class MouseTripleEvent final : public MouseEvent {
public:
    static constexpr EventType static_type() { return EventType::MouseTriple; }

    MouseTripleEvent(int buttons_down, int x, int y, int z, int button, int modifiers)
        : MouseEvent(static_type(), buttons_down, x, y, z, button, modifiers) {}
};

class MouseMoveEvent final : public MouseEvent {
public:
    static constexpr EventType static_type() { return EventType::MouseMove; }

    MouseMoveEvent(int buttons_down, int x, int y, int z, int button, int modifiers)
        : MouseEvent(static_type(), buttons_down, x, y, z, button, modifiers) {}
};

class MouseUpEvent final : public MouseEvent {
public:
    static constexpr EventType static_type() { return EventType::MouseUp; }

    MouseUpEvent(int buttons_down, int x, int y, int z, int button, int modifiers)
        : MouseEvent(static_type(), buttons_down, x, y, z, button, modifiers) {}
};

class MouseScrollEvent final : public MouseEvent {
public:
    static constexpr EventType static_type() { return EventType::MouseScroll; }

    MouseScrollEvent(int buttons_down, int x, int y, int z, int button, int modifiers)
        : MouseEvent(static_type(), buttons_down, x, y, z, button, modifiers) {}
};

class CallbackEvent final : public Event {
public:
    static constexpr EventType static_type() { return EventType::Callback; }

    explicit CallbackEvent(Function<void()> callback) : Event(static_type()), m_callback(move(callback)) {}

    void invoke() { m_callback(); }

private:
    Function<void()> m_callback;
};

class TimerEvent final : public Event {
public:
    static constexpr EventType static_type() { return EventType::Timer; }

    explicit TimerEvent(int times_expired) : Event(static_type()), m_times_expired(times_expired) {}

    int times_expired() const { return m_times_expired; }

private:
    int m_times_expired;
};

class ThemeChangeEvent final : public Event {
public:
    static constexpr EventType static_type() { return EventType::ThemeChange; }

    ThemeChangeEvent() : Event(static_type()) {}
};
}
