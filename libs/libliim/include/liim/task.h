#pragma once

#include <liim/coroutine.h>

namespace LIIM {
template<typename T = void>
class Task;

template<>
class Task<void> {
public:
    struct Promise;

    using Handle = std::coroutine_handle<Promise>;
    using promise_type = Promise;

    struct Promise {
        std::coroutine_handle<> waiter { noop_coroutine() };

        Task get_return_object() { return Task { Handle::from_promise(*this) }; }
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
        void return_void() {}
    };

    struct Waiter {
        Handle handle;

        bool await_ready() { return false; }
        void await_resume() {}
        auto await_suspend(CoroutineHandle<> waiter) {
            handle.promise().waiter = waiter;
            return handle;
        }
    };

    Waiter operator co_await() && { return Waiter { m_handle }; }

    void operator()() { m_handle.resume(); }
    bool finished() const { return !m_handle || m_handle.done(); }

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

template<typename T>
requires(!IsSame<void, T>::value) class Task<T> {
public:
    struct Promise;

    using Handle = std::coroutine_handle<Promise>;
    using promise_type = Promise;

    struct Promise {
        std::coroutine_handle<> waiter { noop_coroutine() };
        T value;

        Task get_return_object() { return Task { Handle::from_promise(*this) }; }
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
        void return_value(T&& object) { value = move(object); }
        void return_value(const T& object) { value = object; }
    };

    struct Waiter {
        Handle handle;

        bool await_ready() { return false; }
        T await_resume() { return move(handle.promise().value); }
        auto await_suspend(CoroutineHandle<> waiter) {
            handle.promise().waiter = waiter;
            return handle;
        }
    };

    Waiter operator co_await() && { return Waiter { m_handle }; }

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

using LIIM::Task;
