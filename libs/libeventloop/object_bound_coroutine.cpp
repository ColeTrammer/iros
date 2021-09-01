#include <eventloop/object.h>
#include <eventloop/object_bound_coroutine.h>

namespace App {
SuspendAlways ObjectBoundCoroutine::Promise::final_suspend() {
    if (auto object = coroutine->m_owner.lock()) {
        object->cleanup_coroutine(coroutine);
    }
    return {};
}

void ObjectBoundCoroutine::set_owner(Object& owner) {
    m_owner = owner.weak_from_this();
}
}
