#pragma once

#include <eventloop/event.h>
#include <eventloop/forward.h>
#include <eventloop/object_bound_coroutine.h>
#include <liim/function.h>
#include <liim/pointers.h>
#include <liim/string_view.h>
#include <liim/task.h>
#include <liim/vector.h>

#define APP_OBJECT(name)                                                                \
public:                                                                                 \
    template<typename... Args>                                                          \
    static SharedPtr<name> create(SharedPtr<Object> parent, Args&&... args) {           \
        auto ret = SharedPtr<name>(new name(forward<Args>(args)...));                   \
        ret->__set_weak_this(WeakPtr<name>(ret));                                       \
        if (parent) {                                                                   \
            parent->add_child(ret);                                                     \
        }                                                                               \
        ret->initialize();                                                              \
        return ret;                                                                     \
    }                                                                                   \
                                                                                        \
    template<typename Ev>                                                               \
    auto block_until_event(Object& coroutine_owner) {                                   \
        static_assert(does_emit<Ev>());                                                 \
        return this->block_until_event_unchecked<Ev>(coroutine_owner);                  \
    }                                                                                   \
                                                                                        \
    template<typename... Ev, typename HandlerCallback>                                  \
    int on(GlobalListenerTag, HandlerCallback&& handler_callback) {                     \
        static_assert(does_emit<Ev...>());                                              \
        return this->on_unchecked<Ev...>(GlobalListenerTag {}, move(handler_callback)); \
    }                                                                                   \
                                                                                        \
    template<typename... Ev, typename HandlerCallback>                                  \
    int on(Object& listener, HandlerCallback&& handler_callback) {                      \
        static_assert(does_emit<Ev...>());                                              \
        return this->on_unchecked<Ev...>(listener, move(handler_callback));             \
    }                                                                                   \
                                                                                        \
protected:                                                                              \
    template<typename... Ev, typename HandlerCallback>                                  \
    int on(HandlerCallback&& handler_callback) {                                        \
        static_assert(does_emit<Ev...>());                                              \
        return this->on_unchecked<Ev...>(move(handler_callback));                       \
    }                                                                                   \
                                                                                        \
private:

#define APP_EMITS(Parent, ...)                                                                 \
public:                                                                                        \
    template<typename... Ev>                                                                   \
    static constexpr bool does_emit() {                                                        \
        return (LIIM::IsOneOf<Ev, ##__VA_ARGS__>::value && ...) || Parent::does_emit<Ev...>(); \
    }                                                                                          \
                                                                                               \
private:

APP_EVENT(App, CallbackEvent, Event, (), ((Function<void()>, callback)), (void invoke() { m_callback(); }))

namespace App {
class Object {
public:
    class GlobalListenerTag {};

private:
    APP_OBJECT(Object)

private:
    class Handler {
    public:
        enum class Bool { Yes };
        enum class Void { Yes };

        Handler(int token, StringView event_name, Bool, Function<bool(const Event&)> handler)
            : m_token(token), m_event_name(event_name), m_is_bool_handler(true), m_bool_handler(move(handler)) {}
        Handler(int token, StringView event_name, Void, Function<void(const Event&)> handler)
            : m_token(token), m_event_name(event_name), m_is_bool_handler(false), m_void_handler(move(handler)) {}

        bool can_handle(const App::Event& event) const;
        bool handle(const App::Event& event);

        void set_listener(WeakPtr<Object> listener);
        SharedPtr<Object> listener() const { return m_listener.lock(); }

        bool global_listener() const { return m_global_listener; }
        int token() const { return m_token; }

    private:
        int m_token { 0 };
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

    template<typename... Ev>
    static constexpr bool does_emit() {
        return false;
    }

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

    template<typename... Ev, typename HandlerCallback>
    int on_unchecked(GlobalListenerTag, HandlerCallback&& handler_callback) {
        return this->on_unchecked<Ev...>(move(handler_callback));
    }

    template<typename... Ev, typename HandlerCallback>
    int on_unchecked(Object& listener, HandlerCallback&& handler_callback) {
        return this->on_unchecked<Ev...>(move(handler_callback), listener.weak_from_this());
    }

    void remove_listener(Object& listener);
    void remove_listener(int token);

    void start_coroutine(ObjectBoundCoroutine&& coroutine);
    void schedule_coroutine(CoroutineHandle<> handle);
    void cleanup_coroutine(ObjectBoundCoroutine* coroutine);

    template<typename Ev>
    auto block_until_event_unchecked(Object& coroutine_owner) {
        class EventWaiter {
        public:
            EventWaiter(Object& target_object, Object& coroutine_owner)
                : m_target_object(&target_object), m_coroutine_owner(&coroutine_owner) {}

            bool await_ready() { return false; }
            Ev await_resume() {
                assert(m_event);
                return *m_event;
            }
            void await_suspend(CoroutineHandle<> handle) {
                m_callback_token = m_target_object->on_unchecked<Ev>(*m_coroutine_owner, [this, handle](const Ev& event) {
                    m_event = event;
                    m_coroutine_owner->schedule_coroutine(handle);
                    m_target_object->deferred_invoke([target = m_target_object, token = m_callback_token] {
                        target->remove_listener(token);
                    });

                    if constexpr (Ev::event_requires_handling()) {
                        return true;
                    }
                });
            }

        private:
            Object* m_target_object { nullptr };
            Object* m_coroutine_owner { nullptr };
            Maybe<Ev> m_event;
            int m_callback_token { 0 };
        };
        return EventWaiter { *this, coroutine_owner };
    }

protected:
    Object();

    virtual void did_add_child(SharedPtr<Object>) {}
    virtual void did_remove_child(SharedPtr<Object>) {}

    template<typename... Ev, typename HandlerCallback>
    int on_unchecked(HandlerCallback&& handler, Maybe<WeakPtr<Object>> listener = {}) {
        auto token = m_next_callback_token++;
        (
            [&] {
                auto callback = [handler](const Event& event) {
                    return handler(static_cast<const Ev&>(event));
                };

                if constexpr (Ev::event_requires_handling()) {
                    static_assert(LIIM::IsSame<bool, typename LIIM::InvokeResult<HandlerCallback, const Ev&>::type>::value,
                                  "Callback handler function must return bool");
                    m_handlers.add(Handler { token, Ev::static_event_name(), Handler::Bool::Yes, move(callback) });
                } else {
                    static_assert(LIIM::IsSame<void, typename LIIM::InvokeResult<HandlerCallback, const Ev&>::type>::value,
                                  "Callback handler function must return void");
                    m_handlers.add(Handler { token, Ev::static_event_name(), Handler::Void::Yes, move(callback) });
                }

                if (listener) {
                    m_handlers.last().set_listener(*listener);
                }
            }(),
            ...);
        return token;
    }

private:
    Vector<SharedPtr<Object>> m_children;
    Vector<Handler> m_handlers;
    Vector<ObjectBoundCoroutine> m_owned_coroutines;
    Object* m_parent { nullptr };
    mutable WeakPtr<Object> m_weak_this;
    int m_next_callback_token { 1 };
};
}
