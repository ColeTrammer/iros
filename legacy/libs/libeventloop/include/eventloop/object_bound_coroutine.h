#pragma once

#include <eventloop/forward.h>
#include <liim/coroutine.h>
#include <liim/pointers.h>

namespace App {
class [[nodiscard]] ObjectBoundCoroutine {
public:
    struct Promise;

    using Handle = CoroutineHandle<Promise>;
    using promise_type = Promise;

    struct Promise {
        WeakPtr<Object> owner;

        ObjectBoundCoroutine get_return_object() { return ObjectBoundCoroutine { Handle::from_promise(*this) }; }
        void unhandled_exception() {}
        SuspendAlways initial_suspend() { return {}; }
        SuspendAlways final_suspend() noexcept;
        void return_void() {}
    };

    ObjectBoundCoroutine(ObjectBoundCoroutine&& other) : m_handle(LIIM::exchange(other.m_handle, nullptr)) {}

    ~ObjectBoundCoroutine() {
        if (m_handle) {
            m_handle.destroy();
        }
    }

    Handle handle() const { return m_handle; }

    void set_owner(Object&);

private:
    explicit ObjectBoundCoroutine(Handle handle) : m_handle(handle) {}

    Handle m_handle;
};
}
