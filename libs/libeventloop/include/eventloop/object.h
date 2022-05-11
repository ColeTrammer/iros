#pragma once

#include <eventloop/event.h>
#include <eventloop/forward.h>
#include <eventloop/object_bound_coroutine.h>
#include <liim/function.h>
#include <liim/pointers.h>
#include <liim/string_view.h>
#include <liim/task.h>
#include <liim/vector.h>

#define APP_OBJECT(name)                                                                       \
public:                                                                                        \
    template<typename... Args>                                                                 \
    static SharedPtr<name> create_without_initializing(Object* parent, Args&&... args) {       \
        auto ret = SharedPtr<name>(new name(forward<Args>(args)...));                          \
        ret->__set_weak_this(WeakPtr<name>(ret));                                              \
        if (parent) {                                                                          \
            parent->add_child(ret);                                                            \
        }                                                                                      \
        return ret;                                                                            \
    }                                                                                          \
                                                                                               \
    template<typename... Args>                                                                 \
    static SharedPtr<name> create(Object* parent, Args&&... args) {                            \
        auto ret = create_without_initializing(parent, forward<Args>(args)...);                \
        ret->initialize();                                                                     \
        return ret;                                                                            \
    }                                                                                          \
                                                                                               \
    name(const name& other) = delete;                                                          \
    name(name&& other) = delete;                                                               \
                                                                                               \
    template<typename Ev>                                                                      \
    auto block_until_event(Object& coroutine_owner) {                                          \
        static_assert(does_emit<Ev>());                                                        \
        return this->block_until_event_unchecked<Ev>(coroutine_owner);                         \
    }                                                                                          \
                                                                                               \
    template<typename... Ev, typename HandlerCallback>                                         \
    int intercept(GlobalListenerTag, HandlerCallback&& handler_callback) {                     \
        static_assert(does_emit<Ev...>());                                                     \
        return this->intercept_unchecked<Ev...>(GlobalListenerTag {}, move(handler_callback)); \
    }                                                                                          \
                                                                                               \
    template<typename... Ev, typename HandlerCallback>                                         \
    int intercept(Object& listener, HandlerCallback&& handler_callback) {                      \
        static_assert(does_emit<Ev...>());                                                     \
        return this->intercept_unchecked<Ev...>(listener, move(handler_callback));             \
    }                                                                                          \
                                                                                               \
    template<typename... Ev, typename HandlerCallback>                                         \
    int on(GlobalListenerTag, HandlerCallback&& handler_callback) {                            \
        static_assert(does_emit<Ev...>());                                                     \
        return this->on_unchecked<Ev...>(GlobalListenerTag {}, move(handler_callback));        \
    }                                                                                          \
                                                                                               \
    template<typename... Ev, typename HandlerCallback>                                         \
    int on(Object& listener, HandlerCallback&& handler_callback) {                             \
        static_assert(does_emit<Ev...>());                                                     \
        return this->on_unchecked<Ev...>(listener, move(handler_callback));                    \
    }                                                                                          \
                                                                                               \
protected:                                                                                     \
    template<typename... Ev, typename HandlerCallback>                                         \
    int intercept(HandlerCallback&& handler_callback) {                                        \
        static_assert(does_emit<Ev...>());                                                     \
        return this->intercept_unchecked<Ev...>(move(handler_callback));                       \
    }                                                                                          \
                                                                                               \
    template<typename... Ev, typename HandlerCallback>                                         \
    int on(HandlerCallback&& handler_callback) {                                               \
        static_assert(does_emit<Ev...>());                                                     \
        return this->on_unchecked<Ev...>(move(handler_callback));                              \
    }                                                                                          \
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

#define APP_OBJECT_FORWARD_EVENT_API(o)                                                                 \
public:                                                                                                 \
    template<typename Ev>                                                                               \
    auto block_until_event(App::Object& coroutine_owner) {                                              \
        static_assert(does_emit<Ev>());                                                                 \
        return o.block_until_event_unchecked<Ev>(coroutine_owner);                                      \
    }                                                                                                   \
                                                                                                        \
    template<typename... Ev, typename HandlerCallback>                                                  \
    int intercept(App::Object::GlobalListenerTag, HandlerCallback&& handler_callback) {                 \
        static_assert(does_emit<Ev...>());                                                              \
        return o.intercept_unchecked<Ev...>(App::Object::GlobalListenerTag {}, move(handler_callback)); \
    }                                                                                                   \
                                                                                                        \
    template<typename... Ev, typename HandlerCallback>                                                  \
    int intercept(App::Object& listener, HandlerCallback&& handler_callback) {                          \
        static_assert(does_emit<Ev...>());                                                              \
        return o.intercept_unchecked<Ev...>(listener, move(handler_callback));                          \
    }                                                                                                   \
                                                                                                        \
    template<typename... Ev, typename HandlerCallback>                                                  \
    int on(App::Object::GlobalListenerTag, HandlerCallback&& handler_callback) {                        \
        static_assert(does_emit<Ev...>());                                                              \
        return o.on_unchecked<Ev...>(App::Object::GlobalListenerTag {}, move(handler_callback));        \
    }                                                                                                   \
                                                                                                        \
    template<typename... Ev, typename HandlerCallback>                                                  \
    int on(App::Object& listener, HandlerCallback&& handler_callback) {                                 \
        static_assert(does_emit<Ev...>());                                                              \
        return o.on_unchecked<Ev...>(listener, move(handler_callback));                                 \
    }                                                                                                   \
                                                                                                        \
protected:                                                                                              \
    template<typename... Ev, typename HandlerCallback>                                                  \
    int intercept(HandlerCallback&& handler_callback) {                                                 \
        static_assert(does_emit<Ev...>());                                                              \
        return o.intercept_unchecked<Ev...>(App::Object::GlobalListenerTag {}, move(handler_callback)); \
    }                                                                                                   \
                                                                                                        \
    template<typename... Ev, typename HandlerCallback>                                                  \
    int on(HandlerCallback&& handler_callback) {                                                        \
        static_assert(does_emit<Ev...>());                                                              \
        return o.on<Ev...>(App::Object::GlobalListenerTag {}, move(handler_callback));                  \
    }                                                                                                   \
                                                                                                        \
private:

#define APP_OBJECT_FORWARD_API(o)                                                                             \
public:                                                                                                       \
    template<typename ObjectType, typename... Args>                                                           \
    SharedPtr<ObjectType> add(Args&&... args) {                                                               \
        return o.add<ObjectType>(forward<Args>(args)...);                                                     \
    }                                                                                                         \
                                                                                                              \
    const Vector<SharedPtr<App::Object>>& children() const { return o.children(); }                           \
                                                                                                              \
    App::Object* parent() { return o.parent(); }                                                              \
    const App::Object* parent() const { return o.parent(); }                                                  \
                                                                                                              \
    void deferred_invoke(Function<void()> callback) { o.deferred_invoke(move(callback)); }                    \
    void deferred_invoke_batched(bool& already_registered_flag, Function<void()> callback) {                  \
        o.deferred_invoke_batched(already_registered_flag, move(callback));                                   \
    }                                                                                                         \
                                                                                                              \
    template<typename Ev, typename... Args>                                                                   \
    bool emit(Args&&... args) const {                                                                         \
        return o.emit<Ev>(forward<Args>(args)...);                                                            \
    }                                                                                                         \
                                                                                                              \
    bool forward_to(const App::Object& to, const App::Event& event) const { return o.forward_to(to, event); } \
    bool forward_to_children(const App::Event& event) const { return o.forward_to_children(event); }          \
                                                                                                              \
    template<typename... Ev, typename HandlerCallback>                                                        \
    int intercept_unchecked(App::Object::GlobalListenerTag, HandlerCallback&& handler_callback) {             \
        return o.intercept_unchecked<Ev...>(App::Object::GlobalListenerTag {}, move(handler_callback));       \
    }                                                                                                         \
                                                                                                              \
    template<typename... Ev, typename HandlerCallback>                                                        \
    int intercept_unchecked(App::Object& listener, HandlerCallback&& handler_callback) {                      \
        return o.intercept_unchecked<Ev...>(listener, move(handler_callback));                                \
    }                                                                                                         \
                                                                                                              \
    template<typename... Ev, typename HandlerCallback>                                                        \
    int on_unchecked(App::Object::GlobalListenerTag, HandlerCallback&& handler_callback) {                    \
        return o.on_unchecked<Ev...>(App::Object::GlobalListenerTag {}, move(handler_callback));              \
    }                                                                                                         \
                                                                                                              \
    template<typename... Ev, typename HandlerCallback>                                                        \
    int on_unchecked(App::Object& listener, HandlerCallback&& handler_callback) {                             \
        return o.on_unchecked<Ev...>(listener, move(handler_callback));                                       \
    }                                                                                                         \
                                                                                                              \
protected:                                                                                                    \
    template<typename... Ev, typename HandlerCallback>                                                        \
    int intercept_unchecked(HandlerCallback&& handler_callback) {                                             \
        return o.intercept_unchecked<Ev...>(App::Object::GlobalListenerTag {}, move(handler_callback));       \
    }                                                                                                         \
                                                                                                              \
    template<typename... Ev, typename HandlerCallback>                                                        \
    int on_unchecked(HandlerCallback&& handler_callback) {                                                    \
        return o.on_unchecked<Ev...>(App::Object::GlobalListenerTag {}, move(handler_callback));              \
    }                                                                                                         \
                                                                                                              \
public:                                                                                                       \
    void remove_listener(App::Object& listener) { o.remove_listener(listener); }                              \
    void remove_listener(int token) { o.remove_listener(token); }                                             \
                                                                                                              \
    void start_coroutine(App::ObjectBoundCoroutine&& coroutine) { o.start_coroutine(move(coroutine)); }       \
                                                                                                              \
    template<typename... Ev, typename ObjectType, typename Callback>                                          \
    int listen(ObjectType& obj, Callback&& cb) {                                                              \
        return obj.template on<Ev...>(o, move(cb));                                                           \
    }                                                                                                         \
                                                                                                              \
    template<typename... Ev, typename ObjectType, typename Callback>                                          \
    int listen_unchecked(ObjectType& obj, Callback&& cb) {                                                    \
        return obj.template on_unchecked<Ev...>(o, move(cb));                                                 \
    }                                                                                                         \
                                                                                                              \
    template<typename... Ev, typename ObjectType, typename Callback>                                          \
    int listen_intercept(ObjectType& obj, Callback&& cb) {                                                    \
        return obj.template intercept<Ev...>(o, move(cb));                                                    \
    }                                                                                                         \
                                                                                                              \
    template<typename... Ev, typename ObjectType, typename Callback>                                          \
    int listen_intercept_unchecked(ObjectType& obj, Callback&& cb) {                                          \
        return obj.template intercept_unchecked<Ev...>(o, move(cb));                                          \
    }                                                                                                         \
                                                                                                              \
    template<typename Ev>                                                                                     \
    auto block_until_event_unchecked(App::Object& coroutine_owner) {                                          \
        return o.block_until_event_unchecked<Ev>(coroutine_owner);                                            \
    }                                                                                                         \
                                                                                                              \
private:

// FIXME: replace with macro when GCC fixes very strange if constexpr bug.
// APP_EVENT(App, CallbackEvent, Event, (), ((Function<void()>, callback)), (void invoke() { m_callback(); }))
namespace App {
class CallbackEvent : public Event {
public:
    static constexpr StringView static_event_name() {
        return ""
               "App"
               "::"
               "CallbackEvent";
    }
    static constexpr bool event_requires_handling() { return 0; }

private:
public:
    explicit CallbackEvent(Function<void()> callback) : Event(static_event_name()), m_callback(move(callback)) {}
    typename App::Detail::GetterType<Function<void()>>::type callback() const { return m_callback; }
    void set_callback(Function<void()> callback) { m_callback = move(callback); }
    void invoke() { m_callback(); }
    virtual Vector<FieldString> field_strings() const override {
        auto vector = Event::field_strings();
        vector.add(FieldString { ""
                                 "callback",
                                 "<>" });
        return vector;
    }

private:
    Function<void()> m_callback;
};
}

namespace App {
class Object {
public:
    class GlobalListenerTag {};

private:
    enum class ListenerOrdering {
        BeforeOthers,
        AfterOthers,
    };

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

    virtual void initialize();

    template<typename... Ev>
    static constexpr bool does_emit() {
        return false;
    }

    void add_child(SharedPtr<Object> child);
    void remove_child(SharedPtr<Object> child);

    template<typename ObjectType, typename... Args>
    SharedPtr<ObjectType> add(Args&&... args) {
        return ObjectType::create(this, forward<Args>(args)...);
    }

    const Vector<SharedPtr<Object>>& children() const { return m_children; }

    virtual bool is_base_widget() const { return false; }
    virtual bool is_window() const { return false; }

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
    int intercept_unchecked(GlobalListenerTag, HandlerCallback&& handler_callback) {
        return this->intercept_unchecked<Ev...>(move(handler_callback));
    }

    template<typename... Ev, typename HandlerCallback>
    int intercept_unchecked(Object& listener, HandlerCallback&& handler_callback) {
        return this->on_unchecked_impl<Ev...>(move(handler_callback), ListenerOrdering::BeforeOthers, listener.weak_from_this());
    }

    template<typename... Ev, typename HandlerCallback>
    int on_unchecked(GlobalListenerTag, HandlerCallback&& handler_callback) {
        return this->on_unchecked<Ev...>(move(handler_callback));
    }

    template<typename... Ev, typename HandlerCallback>
    int on_unchecked(Object& listener, HandlerCallback&& handler_callback) {
        return this->on_unchecked_impl<Ev...>(move(handler_callback), ListenerOrdering::AfterOthers, listener.weak_from_this());
    }

    void remove_listener(Object& listener);
    void remove_listener(int token);

    void start_coroutine(ObjectBoundCoroutine&& coroutine);
    void schedule_coroutine(CoroutineHandle<> handle);
    void cleanup_coroutine(CoroutineHandle<> handle);

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
                m_callback_token = m_target_object->intercept_unchecked<Ev>(*m_coroutine_owner, [this, handle](const Ev& event) {
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
            Option<Ev> m_event;
            int m_callback_token { 0 };
        };
        return EventWaiter { *this, coroutine_owner };
    }

protected:
    Object();

    virtual void did_add_child(SharedPtr<Object>) {}
    virtual void did_remove_child(SharedPtr<Object>) {}

    template<typename... Ev, typename HandlerCallback>
    int intercept_unchecked(HandlerCallback&& handler) {
        return on_unchecked_impl<Ev...>(handler, ListenerOrdering::BeforeOthers);
    }

    template<typename... Ev, typename HandlerCallback>
    int on_unchecked(HandlerCallback&& handler) {
        return on_unchecked_impl<Ev...>(handler, ListenerOrdering::AfterOthers);
    }

    template<typename... Ev, typename ObjectType, typename Callback>
    int listen(ObjectType& obj, Callback&& cb) {
        return obj.template on<Ev...>(*this, move(cb));
    }

    template<typename... Ev, typename ObjectType, typename Callback>
    int listen_unchecked(ObjectType& obj, Callback&& cb) {
        return obj.template on_unchecked<Ev...>(*this, move(cb));
    }

    template<typename... Ev, typename ObjectType, typename Callback>
    int listen_intercept(ObjectType& obj, Callback&& cb) {
        return obj.template intercept<Ev...>(*this, move(cb));
    }

    template<typename... Ev, typename ObjectType, typename Callback>
    int listen_intercept_unchecked(ObjectType& obj, Callback&& cb) {
        return obj.template intercept_unchecked<Ev...>(*this, move(cb));
    }

    template<typename... Ev, typename HandlerCallback>
    int on_unchecked_impl(HandlerCallback&& handler, ListenerOrdering ordering, Option<WeakPtr<Object>> listener = {}) {
        auto token = m_next_callback_token++;
        (
            [&] {
                auto callback = [handler](const Event& event) {
                    return handler(static_cast<const Ev&>(event));
                };

                auto position = ordering == ListenerOrdering::AfterOthers ? m_handlers.size() : 0;
                if constexpr (Ev::event_requires_handling()) {
                    static_assert(LIIM::IsSame<bool, typename LIIM::InvokeResult<HandlerCallback, const Ev&>::type>::value,
                                  "Callback handler function must return bool");
                    m_handlers.insert(Handler { token, Ev::static_event_name(), Handler::Bool::Yes, move(callback) }, position);
                } else {
                    static_assert(LIIM::IsSame<void, typename LIIM::InvokeResult<HandlerCallback, const Ev&>::type>::value,
                                  "Callback handler function must return void");
                    m_handlers.insert(Handler { token, Ev::static_event_name(), Handler::Void::Yes, move(callback) }, position);
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
