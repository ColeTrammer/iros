#pragma once

#include <coroutine>
#include <liim/utilities.h>

namespace LIIM {

struct SupsendAlways {
    bool await_ready() { return false; }
    void await_suspend(std::coroutine_handle<>) {}
    void await_resume() {}
};

struct SuspendNever {
    bool await_ready() { return true; }
    void await_suspend(std::coroutine_handle<>) {}
    void await_resume() {}
};

template<typename T>
class PureGenerator {
public:
    struct Promise;

    using Handle = std::coroutine_handle<Promise>;
    using promise_type = Promise;

    struct Promise {
        T value;

        PureGenerator get_return_object() { return PureGenerator(Handle::from_promise(*this)); }

        SupsendAlways initial_suspend() { return {}; }
        SupsendAlways final_suspend() { return {}; }
        void unhandled_exception() {}
        SupsendAlways yield_value(T v) {
            this->value = move(v);
            return {};
        }
        void return_void() {}
    };

    Handle handle() { return m_handle; }

    ~PureGenerator() { m_handle.destroy(); }

    bool finished() {
        maybe_get_value();
        return m_handle.done();
    }

    T operator()() {
        maybe_get_value();
        m_has_value = false;
        return move(m_handle.promise().value);
    }

private:
    PureGenerator(Handle handle) : m_handle(handle) {}

    void maybe_get_value() {
        if (m_has_value || m_handle.done()) {
            return;
        }

        m_handle();
        m_has_value = true;
    }

    Handle m_handle;
    T m_value;
    bool m_has_value { false };
};

template<typename T>
class Generator {
public:
    struct Promise;

    using Handle = std::coroutine_handle<Promise>;
    using promise_type = Promise;

    struct Promise {
        T value;

        Generator get_return_object() { return Generator(Handle::from_promise(*this)); }

        SupsendAlways initial_suspend() { return {}; }
        SupsendAlways final_suspend() { return {}; }
        void unhandled_exception() {}
        SupsendAlways yield_value(T v) {
            this->value = move(v);
            return {};
        }
        SupsendAlways return_value(T v) {
            this->value = move(v);
            return {};
        }
    };

    Handle handle() { return m_handle; }

    ~Generator() { m_handle.destroy(); }

    bool finished() {
        maybe_get_value();
        return !m_has_value && m_handle.done();
    }

    T operator()() {
        maybe_get_value();
        m_has_value = false;
        return move(m_handle.promise().value);
    }

private:
    Generator(Handle handle) : m_handle(handle) {}

    void maybe_get_value() {
        if (m_has_value || m_handle.done()) {
            return;
        }

        m_handle();
        m_has_value = true;
    }

    Handle m_handle;
    T m_value;
    bool m_has_value { false };
};

}

using LIIM::Generator;
using LIIM::PureGenerator;
