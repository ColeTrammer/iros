#pragma once

#include <liim/coroutine.h>

namespace LIIM {
// NOTE: This diagrams the execution of the following code:
// Generator<int> T1() { co_yield 1; co_yield 2; }
// Generator<int> ROOT() { co_yield T1(); co_yield 3; }
// int main() { auto generator = ROOT(); PRINT(generator()); PRINT(generator()); PRINT(generator()); generator(); return 0; }

// ROOT() -> ROOT::Generator
//     |-> ROOT::Promise()
//         |-> ROOT::Promise.get_return_object() -> ROOT::Generator
//         |-> ROOT::initial_suspend -> SuspendAlways
//     |-> ROOT::Generator() => Handle<ROOT>.resume()
//         |-> ROOT: co_yield T1()
//             |-> T1::Promise()
//                 |-> T1::Promise.get_return_object() -> T1::Generator
//                 |-> T1::initial_suspend -> SuspendAlways
//             |-> co_await ROOT::Promise.yield_value(T1::Generator) -> ROOT::PureGeneratorAwaiter(T1)
//                 |-> await_ready() -> false
//                 |-> await_suspend(Handle<Root>) -> Handle<T1>
//                     |-> T1: co_yield 1
//                         |-> co_await T1::Promise.yield_value(1) -> SuspendAlways
//     |-> PRINT(1)
//     |-> ROOT::Generator() => Handle<T1>.resume()
//         |-> T1: co_yield 2
//             |-> co_await T1::Promise.yield_value(2) -> SuspendAlways
//     |-> PRINT(2)
//     |-> ROOT::Generator() => Handle<T1>.resume()
//         |-> T1: co_return
//             |-> T1::Promise.return_void()
//             |-> co_await T1::Promise.final_suspend() -> RedirectAwaiter(Handle<ROOT>)
//                 |-> await_suspend(Handle<T1>) -> Handle<ROOT>
//                     |-> ROOT: co_yield 3
//                         |-> co_await ROOT::Promise.yield_value(3) -> SuspendAlways
//     |-> PRINT(3)
//     |-> ROOT::Generator() => Handle<ROOT>.resume()
//         |-> Root: co_return
//             |-> ROOT::Promise.return_void()
//             |-> co_await ROOT::Promise.final_suspend() -> RedirectAwaiter(nullptr)
//                 |-> await_suspend(Handle<ROOT>) -> std::noop_coroutine
//     |-> return 0

template<typename T>
class Generator {
public:
    struct Promise;

    using Handle = std::coroutine_handle<Promise>;
    using promise_type = Promise;

    struct Promise {
        T value;
        Handle child_handle { nullptr };
        Handle parent_handle { nullptr };
        bool returned { false };

        Generator get_return_object() { return Generator(*this); }

        SuspendAlways initial_suspend() { return {}; }
        void unhandled_exception() {}
        SuspendAlways yield_value(T v) {
            this->value = move(v);
            return {};
        }

        class PureGeneratorAwaiter {
        public:
            PureGeneratorAwaiter(Handle child) : m_child(child) {}

            bool await_ready() { return false; }
            Handle await_suspend(Handle parent) {
                m_child.promise().parent_handle = parent;
                return m_child;
            }
            void await_resume() {}

        private:
            Handle m_child;
        };

        PureGeneratorAwaiter yield_value(Generator generator) {
            // FIXME: support more than 1 layer of recursion
            assert(!parent_handle);

            child_handle = generator.m_root_handle;
            return PureGeneratorAwaiter(generator.m_root_handle);
        }

        struct RedirectAwaiter {
            std::coroutine_handle<> handle;

            bool await_ready() { return false; }
            std::coroutine_handle<> await_suspend(std::coroutine_handle<>) {
                if (!handle) {
                    return std::noop_coroutine();
                }
                return handle;
            }
            void await_resume() {}
        };

        RedirectAwaiter final_suspend() noexcept {
            if (parent_handle) {
                parent_handle.promise().child_handle = nullptr;
            }
            return { parent_handle };
        }
        void return_void() { returned = true; }
    };

    Generator(Generator&& other)
        : m_root_handle(exchange(other.m_root_handle, nullptr))
        , m_child_handle(exchange(other.m_child_handle, nullptr))
        , m_has_value(exchange(other.m_has_value, false)) {}

    ~Generator() {
        if (m_child_handle && *m_child_handle) {
            m_child_handle->destroy();
        }
        if (m_root_handle) {
            m_root_handle.destroy();
        }
    }

    bool finished() {
        maybe_get_value();
        return !m_has_value;
    }

    T operator()() {
        maybe_get_value();
        m_has_value = false;
        return value();
    }

private:
    Generator(Promise& promise) : m_root_handle(Handle::from_promise(promise)), m_child_handle(&promise.child_handle) {}

    T value() {
        if (*m_child_handle) {
            return m_child_handle->promise().value;
        }
        return m_root_handle.promise().value;
    }

    void maybe_get_value() {
        if (m_has_value || m_root_handle.done()) {
            return;
        }

        if (*m_child_handle) {
            (*m_child_handle)();
        } else {
            m_root_handle();
        }
        m_has_value = !m_root_handle.promise().returned;
    }

    Handle m_root_handle;
    Handle* m_child_handle;
    bool m_has_value { false };
};
}

using LIIM::Generator;
