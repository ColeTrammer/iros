#include <eventloop/object.h>
#include <eventloop/object_bound_coroutine.h>

namespace App {
SuspendAlways ObjectBoundCoroutine::Promise::final_suspend() {
    if (auto object = owner.lock()) {
        object->cleanup_coroutine(Handle::from_promise(*this));
    }
    return {};
}

void ObjectBoundCoroutine::set_owner(Object& owner) {
    m_handle.promise().owner = owner.weak_from_this();
}
}
