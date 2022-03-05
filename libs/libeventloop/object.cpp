#include <eventloop/component.h>
#include <eventloop/event.h>
#include <eventloop/event_loop.h>
#include <eventloop/object.h>

namespace App {
void Object::add_child(SharedPtr<Object> child) {
    child->set_parent(this);
    m_children.add(move(child));
    did_add_child(child);
}

void Object::remove_child(SharedPtr<Object> child) {
    child->set_parent(nullptr);
    m_children.remove_element(child);
    did_remove_child(child);
}

Object::Object() {}

Object::~Object() {
    for (auto& child : m_children) {
        child->set_parent(nullptr);
    }
}

void Object::initialize() {
    m_components.for_each_reverse([&](auto* component) {
        component->attach();
    });
}

void Object::register_component(Component& component) {
    m_components.add(&component);
}

void Object::deferred_invoke(Function<void()> callback) {
    EventLoop::queue_event(weak_from_this(), make_unique<CallbackEvent>(move(callback)));
}

void Object::deferred_invoke_batched(bool& already_registered_flag, Function<void()> callback) {
    if (already_registered_flag) {
        return;
    }

    EventLoop::queue_event(weak_from_this(), make_unique<CallbackEvent>([&already_registered_flag, callback = move(callback)] {
                               callback();
                               already_registered_flag = false;
                           }));
    already_registered_flag = true;
}

bool Object::Handler::can_handle(const Event& event) const {
    return event.name() == m_event_name;
}

bool Object::Handler::handle(const Event& event) {
    if (m_bool_handler) {
        return m_bool_handler(event);
    }
    m_void_handler(event);
    return false;
}

void Object::Handler::set_listener(WeakPtr<Object> listener) {
    m_listener = listener;
    m_global_listener = false;
}

void Object::remove_listener(Object& listener) {
    m_handlers.remove_if([&](auto& handler) {
        return handler.listener().get() == &listener;
    });
}

void Object::remove_listener(int token) {
    m_handlers.remove_if([&](auto& handler) {
        return handler.token() == token;
    });
}

bool Object::dispatch(const Event& event) const {
    auto protector = shared_from_this();

    auto& handlers = const_cast<Vector<Handler>&>(m_handlers);
    for (auto& handler : handlers) {
        if (!handler.can_handle(event)) {
            continue;
        }

        if (handler.global_listener()) {
            if (handler.handle(event)) {
                return true;
            }
            continue;
        }

        if (auto listener = handler.listener()) {
            if (handler.handle(event)) {
                return true;
            }
            continue;
        }
    }

    // FIXME: there's probably a better way to implement GC'ing stale listener callbacks.
    handlers.remove_if([](const Handler& handler) {
        return !handler.global_listener() && !handler.listener();
    });

    return false;
}

void Object::start_coroutine(ObjectBoundCoroutine&& coroutine) {
    m_owned_coroutines.add(move(coroutine));
    m_owned_coroutines.last().set_owner(*this);
    schedule_coroutine(m_owned_coroutines.last().handle());
}

void Object::schedule_coroutine(CoroutineHandle<> handle) {
    deferred_invoke([this, handle] {
        handle();
    });
}

void Object::cleanup_coroutine(CoroutineHandle<> handle) {
    deferred_invoke([this, handle] {
        m_owned_coroutines.remove_if([&](auto& element) {
            return handle == element.handle();
        });
    });
}
}
