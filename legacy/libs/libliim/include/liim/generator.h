#pragma once

#include <liim/container/container.h>
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
class Generator : public ValueIteratorAdapter<Generator<T>> {
public:
    struct Promise;

    using ValueType = T;
    using Handle = CoroutineHandle<Promise>;
    using promise_type = Promise;

    struct Promise {
        Option<T> value;
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
            child_handle = generator.m_root_handle;
            return PureGeneratorAwaiter(generator.m_root_handle);
        }

        struct RedirectAwaiter {
            CoroutineHandle<> handle;

            bool await_ready() { return false; }
            CoroutineHandle<> await_suspend(CoroutineHandle<>) {
                if (!handle) {
                    return noop_coroutine();
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
        : m_root_handle(LIIM::exchange(other.m_root_handle, nullptr)), m_has_value(exchange(other.m_has_value, false)) {}

    ~Generator() {
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

    Option<T> next() {
        if (finished()) {
            return {};
        }
        return (*this)();
    }

private:
    Generator(Promise& promise) : m_root_handle(Handle::from_promise(promise)) {}

    T value() {
        auto* child = current_child();
        if (*child) {
            return *move(child->promise().value);
        }
        return *move(m_root_handle.promise().value);
    }

    void maybe_get_value() {
        if (m_has_value || m_root_handle.done()) {
            return;
        }

        auto* child = current_child();
        assert(*child);
        (*child)();
        m_has_value = !m_root_handle.promise().returned;
    }

    Handle* current_child() {
        Handle* current = &m_root_handle;
        while (current->promise().child_handle) {
            current = &current->promise().child_handle;
        }
        return current;
    }

    Handle m_root_handle;
    bool m_has_value { false };
};
}

using LIIM::Generator;
