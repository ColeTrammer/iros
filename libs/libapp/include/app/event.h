#pragma once

#include <kernel/hal/input.h>

namespace App {

class Event {
public:
    enum class Type {
        Invalid,
        Key,
        Mouse,
        Window,
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
    };

    WindowEvent(Type type) : Event(Event::Type::Window), m_type(type) {}

    Type window_event_type() const { return m_type; }

private:
    Type m_type { Type::Invalid };
};

class MouseEvent final : public Event {
public:
    MouseEvent(int x, int y, scroll_state scroll, mouse_button_state left, mouse_button_state right)
        : Event(Event::Type::Mouse), m_x(x), m_y(y), m_scroll(scroll), m_left(left), m_right(right) {}

    int x() const { return m_x; }
    int y() const { return m_y; }
    scroll_state scroll() const { return m_scroll; }
    mouse_button_state left() const { return m_left; }
    mouse_button_state right() const { return m_right; }

    void set_x(int x) { m_x = x; }
    void set_y(int y) { m_y = y; }

private:
    int m_x { 0 };
    int m_y { 0 };
    scroll_state m_scroll { SCROLL_NONE };
    mouse_button_state m_left { MOUSE_NO_CHANGE };
    mouse_button_state m_right { MOUSE_NO_CHANGE };
};

}
