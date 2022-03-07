#pragma once

#include <assert.h>
#include <eventloop/forward.h>

namespace App {
class Component {
public:
    Component() {}
    virtual ~Component() {}

    Object& object() const {
        assert(m_object);
        return *m_object;
    }

    template<typename T>
    T& typed_object() const {
        return static_cast<T&>(object());
    }

    void attach(Object& object) {
        m_object = &object;
        did_attach();
    }

protected:
    virtual void did_attach() {}

private:
    Object* m_object;
};
}
