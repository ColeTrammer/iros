#pragma once

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

}
