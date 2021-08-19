#pragma once

#include <eventloop/forward.h>
#include <liim/function.h>
#include <liim/pointers.h>
#include <liim/string_view.h>
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
        ret->initialize();                                                    \
        return ret;                                                           \
    }                                                                         \
                                                                              \
private:

namespace App {
class Object {
    APP_OBJECT(Object)

public:
    virtual ~Object();

    virtual void initialize() {}

    void add_child(SharedPtr<Object> child);
    void remove_child(SharedPtr<Object> child);

    template<typename ObjectType, typename... Args>
    ObjectType& add(Args&&... args) {
        return *ObjectType::create(shared_from_this(), forward<Args>(args)...);
    }

    const Vector<SharedPtr<Object>>& children() const { return m_children; }

    virtual bool is_widget() const { return false; }
    virtual bool is_window() const { return false; }
    virtual bool is_panel() const { return false; }

    Object* parent() { return m_parent; }
    const Object* parent() const { return m_parent; }

    void set_parent(Object* parent) { m_parent = parent; }

    SharedPtr<Object> shared_from_this() { return m_weak_this.lock(); }
    SharedPtr<const Object> shared_from_this() const { return m_weak_this.lock(); }

    WeakPtr<Object> weak_from_this() { return m_weak_this; }
    WeakPtr<const Object> weak_from_this() const { return m_weak_this; }

    void deferred_invoke(Function<void()> callback);

    void __set_weak_this(WeakPtr<Object> weak_this) {
        assert(m_weak_this.expired());
        m_weak_this = move(weak_this);
    }

    bool dispatch(const Event& event) const;

    template<typename Ev, typename HandlerCallback>
    Object& on(HandlerCallback&& handler) {
        static_assert(LIIM::IsSame<bool, typename LIIM::InvokeResult<HandlerCallback, Ev>::type>::value,
                      "Callback handler function must return bool");
        m_handlers.add(Handler { Ev::static_event_name(), [handler = move(handler)](const Event& event) -> bool {
                                    return handler(static_cast<const Ev&>(event));
                                } });
        return *this;
    }

protected:
    Object();

    virtual void did_add_child(SharedPtr<Object>) {}
    virtual void did_remove_child(SharedPtr<Object>) {}

private:
    class Handler {
    public:
        Handler(StringView event_name, Function<bool(const Event&)> handler) : m_event_name(event_name), m_handler(move(handler)) {}

        bool can_handle(const App::Event& event) const;
        bool handle(const App::Event& event);

    private:
        StringView m_event_name;
        Function<bool(const Event&)> m_handler;
    };

    Vector<SharedPtr<Object>> m_children;
    Vector<Handler> m_handlers;
    Object* m_parent;
    mutable WeakPtr<Object> m_weak_this;
};
}
