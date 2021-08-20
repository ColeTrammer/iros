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

Object::~Object() {}

void Object::deferred_invoke(Function<void()> callback) {
    EventLoop::queue_event(weak_from_this(), make_unique<CallbackEvent>(move(callback)));
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

bool Object::dispatch(const Event& event) const {
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
}
