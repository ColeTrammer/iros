#pragma once

#include <di/util/coroutine.h>
#include <di/util/exchange.h>
#include <di/util/unreachable.h>
#include <di/vocab/error/prelude.h>
#include <di/vocab/variant/prelude.h>

namespace di::execution {
namespace lazy_ns {
    template<typename T = void>
    class Lazy;

    template<typename T>
    class PromiseBase {
    public:
        PromiseBase() = default;

        SuspendAlways initial_suspend() noexcept { return {}; }
        auto final_suspend() noexcept { return FinalAwaiter {}; }

        void return_value(T&& value) { m_data.template emplace<1>(util::forward<T>(value)); }

        void unhandled_exception() { util::unreachable(); }

    private:
        template<typename>
        friend class Lazy;

        struct FinalAwaiter {
            bool await_ready() noexcept { return false; }

            template<typename Promise>
            CoroutineHandle<> await_suspend(CoroutineHandle<Promise> coroutine) noexcept {
                PromiseBase& current = coroutine.promise();
                return current.m_continuation ? current.m_continuation : noop_coroutine();
            }

            void await_resume() noexcept {}
        };

        struct Awaiter {
            CoroutineHandle<PromiseBase> coroutine;

            bool await_ready() noexcept { return !coroutine; }

            CoroutineHandle<PromiseBase> await_suspend(CoroutineHandle<> continuation) noexcept {
                coroutine.promise().m_continuation = continuation;
                return coroutine;
            }

            T await_resume() {
                auto& promise = static_cast<PromiseBase&>(coroutine.promise());
                DI_ASSERT(promise.m_data.index() == 1);
                return util::get<1>(util::move(promise.m_data));
            }
        };

        Variant<Void, T, Error> m_data;
        CoroutineHandle<> m_continuation;
    };

    template<>
    class PromiseBase<void> {
    public:
        PromiseBase() = default;

        SuspendAlways initial_suspend() noexcept { return {}; }
        auto final_suspend() noexcept { return FinalAwaiter {}; }

        void return_void() noexcept {}
        void unhandled_exception() { util::unreachable(); }

    private:
        template<typename>
        friend class Lazy;

        struct FinalAwaiter {
            bool await_ready() noexcept { return false; }

            template<typename Promise>
            CoroutineHandle<> await_suspend(CoroutineHandle<Promise> coroutine) noexcept {
                PromiseBase& current = coroutine.promise();
                return current.m_continuation ? current.m_continuation : noop_coroutine();
            }

            void await_resume() noexcept {}
        };

        struct Awaiter {
            CoroutineHandle<PromiseBase> coroutine;

            bool await_ready() noexcept { return !coroutine; }

            CoroutineHandle<PromiseBase> await_suspend(CoroutineHandle<> continuation) noexcept {
                coroutine.promise().m_continuation = continuation;
                return coroutine;
            }

            void await_resume() {}
        };

        CoroutineHandle<> m_continuation;
    };

    template<typename T>
    class [[nodiscard]] Lazy {
    private:
        using PromiseBase = lazy_ns::PromiseBase<T>;

        struct Promise : PromiseBase {
            Lazy get_return_object() noexcept { return Lazy { CoroutineHandle<Promise>::from_promise(*this) }; }
        };

        using Handle = CoroutineHandle<Promise>;
        using ParentHandle = CoroutineHandle<PromiseBase>;
        using Awaiter = PromiseBase::Awaiter;

    public:
        using promise_type = Promise;

        Lazy(Lazy&& other) : m_handle(util::exchange(other.m_handle, {})) {}

        ~Lazy() {
            if (m_handle) {
                m_handle.destroy();
            }
        }

        Awaiter operator co_await() {
            auto& promise = static_cast<PromiseBase&>(m_handle.promise());
            return Awaiter { ParentHandle::from_promise(promise) };
        }

    private:
        explicit Lazy(Handle handle) : m_handle(handle) {}

        Handle m_handle;
    };
}

using lazy_ns::Lazy;
}