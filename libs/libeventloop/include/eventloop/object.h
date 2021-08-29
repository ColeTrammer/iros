#pragma once

#include <eventloop/event.h>
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

APP_EVENT(App, CallbackEvent, Event, (), ((Function<void()>, callback)), (void invoke() { m_callback(); }))

namespace App {
class Object {
    APP_OBJECT(Object)

private:
    class Handler {
    public:
        enum class Bool { Yes };
        enum class Void { Yes };

        Handler(StringView event_name, Bool, Function<bool(const Event&)> handler)
            : m_event_name(event_name), m_is_bool_handler(true), m_bool_handler(move(handler)) {}
        Handler(StringView event_name, Void, Function<void(const Event&)> handler)
            : m_event_name(event_name), m_is_bool_handler(false), m_void_handler(move(handler)) {}

        bool can_handle(const App::Event& event) const;
        bool handle(const App::Event& event);

        void set_listener(WeakPtr<Object> listener);
        SharedPtr<Object> listener() const { return m_listener.lock(); }

        bool global_listener() const { return m_global_listener; }

    private:
        StringView m_event_name;
        bool m_global_listener { true };
        bool m_is_bool_handler { false };
        Function<bool(const Event&)> m_bool_handler;
        Function<void(const Event&)> m_void_handler;
        WeakPtr<Object> m_listener;
    };

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
    virtual bool is_base_widget() const { return false; }
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
    void deferred_invoke_batched(bool& already_registered_flag, Function<void()> callback);

    void __set_weak_this(WeakPtr<Object> weak_this) {
        assert(m_weak_this.expired());
        m_weak_this = move(weak_this);
    }

    template<typename Ev, typename... Args>
    bool emit(Args&&... args) const {
        return dispatch(Ev { forward<Args>(args)... });
    }

    bool forward_to(const Object& to, const Event& event) const { return to.dispatch(event); }

    bool forward_to_children(const Event& event) const {
        for (auto& child : m_children) {
            if (child->dispatch(event)) {
                return true;
            }
        }
        return false;
    }

    bool dispatch(const Event& event) const;

    class GlobalListenerTag {};

    template<typename... Ev, typename HandlerCallback>
    Object& on(GlobalListenerTag, HandlerCallback&& handler_callback) {
        return this->on<Ev...>(move(handler_callback));
    }

    template<typename... Ev, typename HandlerCallback>
    Object& on(Object& listener, HandlerCallback&& handler_callback) {
        return this->on<Ev...>(move(handler_callback), listener.weak_from_this());
    }

    void remove_listener(Object& listener);

protected:
    Object();

    virtual void did_add_child(SharedPtr<Object>) {}
    virtual void did_remove_child(SharedPtr<Object>) {}

    template<typename... Ev, typename HandlerCallback>
    Object& on(HandlerCallback&& handler, Maybe<WeakPtr<Object>> listener = {}) {
        (
            [&] {
                auto callback = [handler](const Event& event) {
                    return handler(static_cast<const Ev&>(event));
                };

                if constexpr (Ev::event_requires_handling()) {
                    static_assert(LIIM::IsSame<bool, typename LIIM::InvokeResult<HandlerCallback, const Ev&>::type>::value,
                                  "Callback handler function must return bool");
                    m_handlers.add(Handler { Ev::static_event_name(), Handler::Bool::Yes, move(callback) });
                } else {
                    static_assert(LIIM::IsSame<void, typename LIIM::InvokeResult<HandlerCallback, const Ev&>::type>::value,
                                  "Callback handler function must return void");
                    m_handlers.add(Handler { Ev::static_event_name(), Handler::Void::Yes, move(callback) });
                }

                if (listener) {
                    m_handlers.last().set_listener(*listener);
                }
            }(),
            ...);
        return *this;
    }

private:
    Vector<SharedPtr<Object>> m_children;
    Vector<Handler> m_handlers;
    Object* m_parent { nullptr };
    mutable WeakPtr<Object> m_weak_this;
};
}
