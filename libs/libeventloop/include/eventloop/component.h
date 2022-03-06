#pragma once

#include <eventloop/forward.h>

namespace App {
class Component {
public:
    virtual ~Component() {}

    Object& object() const { return m_object; }

    template<typename T>
    T& typed_object() const {
        return static_cast<T&>(m_object);
    }

    void attach() { did_attach(); }

protected:
    explicit Component(Object&);

    virtual void did_attach() {}

private:
    Object& m_object;
};
}
