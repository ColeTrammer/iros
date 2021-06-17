#pragma once

#include <assert.h>
#include <coroutine>
#include <liim/utilities.h>

namespace LIIM {
struct SuspendAlways {
    bool await_ready() { return false; }
    void await_suspend(std::coroutine_handle<>) {}
    void await_resume() {}
};

struct SuspendNever {
    bool await_ready() { return true; }
    void await_suspend(std::coroutine_handle<>) {}
    void await_resume() {}
};

template<typename T = void>
class Task;

template<>
class Task<void> {
public:
    struct Promise;

    using Handle = std::coroutine_handle<Promise>;
    using promise_type = Promise;

    struct Promise {
        std::coroutine_handle<> waiter;
        bool returned { false };

        Task get_return_object() { return Task(Handle::from_promise(*this)); }
        void unhandled_exception() {}
        SuspendAlways initial_suspend() { return {}; }
        auto final_suspend() noexcept {
            struct FinalWaiter {
                bool await_ready() { return false; }
                void await_resume() {}
                auto await_suspend(Handle self_handle) { return self_handle.promise().waiter; }
            };
            return FinalWaiter {};
        }
        void return_void() { returned = true; }
    };

    bool await_ready() { return false; }
    void await_resume() {}
    void await_suspend(std::coroutine_handle<> waiter) {
        m_handle.promise().waiter = waiter;
        m_handle.resume();
    }

    void operator()() { m_handle.resume(); }

    Task(Task&& other) : m_handle(other.m_handle) { other.m_handle = nullptr; }

    Task(const Task& other) = delete;

    ~Task() {
        if (m_handle) {
            m_handle.destroy();
        }
    };

private:
    explicit Task(Handle handle) : m_handle(handle) {}

    Handle m_handle;
};
}

using LIIM::SuspendAlways;
using LIIM::SuspendNever;
using LIIM::Task;
