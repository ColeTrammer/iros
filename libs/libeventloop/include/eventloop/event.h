#pragma once

#include <liim/function.h>
#include <kernel/hal/input.h>

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

class MouseEvent final : public Event {
public:
    MouseEvent(MouseEventType mouse_event_type, int buttons_down, int x, int y, int z, int button)
        : Event(Event::Type::Mouse)
        , m_x(x)
        , m_y(y)
        , m_z(z)
        , m_buttons_down(buttons_down)
        , m_button(button)
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

private:
    int m_x { 0 };
    int m_y { 0 };
    int m_z { 0 };
    int m_buttons_down { 0 };
    int m_button { 0 };
    MouseEventType m_mouse_event_type { MouseEventType::Move };
};

class KeyEvent final : public Event {
public:
    KeyEvent(char ascii, key k, int flags) : Event(Event::Type::Key), m_ascii(ascii), m_key(k), m_flags(flags) {}

    char ascii() const { return m_ascii; }
    enum key key() const { return m_key; }
    int flags() const { return m_flags; }

    bool key_up() const { return !!(m_flags & KEY_UP); }
    bool key_down() const { return !!(m_flags & KEY_DOWN); }
    bool control_down() const { return !!(m_flags & KEY_CONTROL_ON); }
    bool shift_down() const { return !!(m_flags & KEY_SHIFT_ON); }
    bool alt_down() const { return !!(m_flags & KEY_ALT_ON); }

private:
    char m_ascii;
    enum key m_key;
    int m_flags;
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
