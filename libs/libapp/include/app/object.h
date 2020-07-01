#pragma once

#include <liim/pointers.h>
#include <liim/vector.h>

#define APP_OBJECT(name)                                                      \
public:                                                                       \
    template<typename... Args>                                                \
    static SharedPtr<name> create(SharedPtr<Object> parent, Args&&... args) { \
        auto ret = SharedPtr<name>(new name(forward<Args>(args)...));         \
        ret->__set_weak_this(WeakPtr<name>(ret));                             \
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
    void remove_child(SharedPtr<Object> child);

    const Vector<SharedPtr<Object>> children() const { return m_children; }

    virtual bool is_widget() const { return false; }
    virtual bool is_window() const { return false; }

    Object* parent() { return m_parent; }
    const Object* parent() const { return m_parent; }

    void set_parent(Object* parent) { m_parent = parent; }

    virtual void on_event(Event&) {}

    SharedPtr<Object> shared_from_this() { return m_weak_this.lock(); }
    SharedPtr<const Object> shared_from_this() const { return m_weak_this.lock(); }

    WeakPtr<Object> weak_from_this() { return m_weak_this; }
    WeakPtr<const Object> weak_from_this() const { return m_weak_this; }

    void __set_weak_this(WeakPtr<Object> weak_this) {
        assert(m_weak_this.expired());
        m_weak_this = move(weak_this);
    }

protected:
    Object();

private:
    Vector<SharedPtr<Object>> m_children;
    Object* m_parent;
    mutable WeakPtr<Object> m_weak_this;
};

}
