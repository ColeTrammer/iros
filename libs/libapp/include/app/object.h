#pragma once

#include <liim/pointers.h>
#include <liim/vector.h>

#define APP_OBJECT(name)                                                      \
public:                                                                       \
    template<typename... Args>                                                \
    static SharedPtr<name> create(SharedPtr<Object> parent, Args&&... args) { \
        auto ret = SharedPtr<name>(new name(forward<Args>(args)...));         \
        if (parent) {                                                         \
            parent->add_child(ret);                                           \
        }                                                                     \
        return ret;                                                           \
    }                                                                         \
                                                                              \
private:

namespace App {

class Event;

class Object {
public:
    virtual ~Object();

    void add_child(SharedPtr<Object> child);

    const Vector<SharedPtr<Object>> children() const { return m_children; }

    virtual bool is_widget() const { return false; }
    virtual bool is_window() const { return false; }

    Object* parent() { return m_parent; }
    const Object* parent() const { return m_parent; }

    void set_parent(Object* parent) { m_parent = parent; }

    virtual void on_event(Event&) {}

protected:
    Object();

private:
    Vector<SharedPtr<Object>> m_children;
    Object* m_parent;
};

}
