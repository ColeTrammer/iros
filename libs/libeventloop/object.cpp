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

}
