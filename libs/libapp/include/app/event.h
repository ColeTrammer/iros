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

private:
    Type m_type { Type::Invalid };
};

}
