#pragma once

#include <liim/function.h>
#include <liim/string.h>

namespace App {
class Event {
public:
    enum class Type {
        Invalid,
        Key,
        Mouse,
        Resize,
        Window,
        WindowState,
        Callback,
        Timer,
        ThemeChange,
    };

    Event(Type type) : m_type(type) {};
    virtual ~Event() {}

    Type type() const { return m_type; }

private:
    Type m_type { Type::Invalid };
};

class WindowEvent final : public Event {
public:
    enum class Type {
        Invalid,
        Close,
        ForceRedraw,
        DidResize,
    };

    WindowEvent(Type type) : Event(Event::Type::Window), m_type(type) {}

    Type window_event_type() const { return m_type; }

private:
    Type m_type { Type::Invalid };
};

class WindowStateEvent final : public Event {
public:
    WindowStateEvent(bool active) : Event(Event::Type::WindowState), m_active(active) {}

    bool active() const { return m_active; }
    void set_active(bool b) { m_active = b; }

private:
    bool m_active;
};

enum class MouseEventType {
    Down,
    Double,
    Triple,
    Move,
    Up,
    Scroll,
};

namespace MouseButton {
    enum {
        None = 0,
        Left = 1,
        Right = 2,
        Middle = 4,
    };
}

enum class KeyEventType {
    Down,
    Up,
};

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

class KeyEvent final : public Event {
public:
    KeyEvent(KeyEventType type, String text, Key key, int modifiers)
        : Event(Event::Type::Key), m_type(type), m_text(move(text)), m_key(key), m_modifiers(modifiers) {}

    const String& text() const { return m_text; }
    Key key() const { return m_key; }
    int modifiers() const { return m_modifiers; }

    bool key_down() const { return m_type == KeyEventType::Down; }
    bool key_up() const { return m_type == KeyEventType::Up; }

    bool shift_down() const { return !!(m_modifiers & KeyModifier::Shift); }
    bool alt_down() const { return !!(m_modifiers & KeyModifier::Alt); }
    bool control_down() const { return !!(m_modifiers & KeyModifier::Control); }
    bool meta_down() const { return !!(m_modifiers & KeyModifier::Meta); }

private:
    KeyEventType m_type;
    String m_text;
    Key m_key;
    int m_modifiers;
};

class MouseEvent final : public Event {
public:
    MouseEvent(MouseEventType mouse_event_type, int buttons_down, int x, int y, int z, int button, int modifiers)
        : Event(Event::Type::Mouse)
        , m_x(x)
        , m_y(y)
        , m_z(z)
        , m_buttons_down(buttons_down)
        , m_button(button)
        , m_modifiers(modifiers)
        , m_mouse_event_type(mouse_event_type) {}

    int x() const { return m_x; }
    int y() const { return m_y; }
    int z() const { return m_z; }
    int button() const { return m_button; }

    void set_x(int x) { m_x = x; }
    void set_y(int y) { m_y = y; }

    MouseEventType mouse_event_type() const { return m_mouse_event_type; }
    int buttons_down() const { return m_buttons_down; };

    bool left_button() const { return m_button == MouseButton::Left; }
    bool right_button() const { return m_button == MouseButton::Right; }
    bool middle_button() const { return m_button == MouseButton::Middle; }

    bool mouse_down_any() const {
        return m_mouse_event_type == MouseEventType::Down || m_mouse_event_type == MouseEventType::Double ||
               m_mouse_event_type == MouseEventType::Triple;
    }

    bool mouse_down() const { return m_mouse_event_type == MouseEventType::Down; }
    bool mouse_double() const { return m_mouse_event_type == MouseEventType::Double; }
    bool mouse_triple() const { return m_mouse_event_type == MouseEventType::Triple; }
    bool mouse_up() const { return m_mouse_event_type == MouseEventType::Up; }
    bool mouse_move() const { return m_mouse_event_type == MouseEventType::Move; }
    bool mouse_scroll() const { return m_mouse_event_type == MouseEventType::Scroll; }

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
    MouseEventType m_mouse_event_type { MouseEventType::Move };
};

class CallbackEvent final : public Event {
public:
    CallbackEvent(Function<void()> callback) : Event(Event::Type::Callback), m_callback(move(callback)) {}

    void invoke() { m_callback(); }

private:
    Function<void()> m_callback;
};

class TimerEvent final : public Event {
public:
    TimerEvent(int times_expired) : Event(Event::Type::Timer), m_times_expired(times_expired) {}

    int times_expired() const { return m_times_expired; }

private:
    int m_times_expired;
};

class ThemeChangeEvent final : public Event {
public:
    ThemeChangeEvent() : Event(Event::Type::ThemeChange) {}
};
}
