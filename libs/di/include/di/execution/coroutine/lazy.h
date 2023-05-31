#pragma once

#include <di/assert/assert_bool.h>
#include <di/concepts/same_as.h>
#include <di/concepts/unexpected.h>
#include <di/execution/algorithm/just.h>
#include <di/execution/algorithm/just_or_error.h>
#include <di/execution/coroutine/with_await_transform.h>
#include <di/execution/coroutine/with_awaitable_senders.h>
#include <di/execution/types/prelude.h>
#include <di/platform/prelude.h>
#include <di/util/coroutine.h>
#include <di/util/exchange.h>
#include <di/util/unreachable.h>
#include <di/vocab/error/error.h>
#include <di/vocab/error/prelude.h>
#include <di/vocab/variant/prelude.h>

namespace di::execution {
namespace lazy_ns {
    template<typename T = void>
    class Lazy;

    struct AllocFailed {};

    template<typename Self, typename T>
    class PromiseBase : public WithAwaitableSenders<Self> {
    public:
        PromiseBase() = default;

        void* operator new(usize size) noexcept { return ::operator new(size, std::nothrow); }
        void operator delete(void* ptr, usize size) noexcept { ::operator delete(ptr, size); }

        SuspendAlways initial_suspend() noexcept { return {}; }
        auto final_suspend() noexcept { return FinalAwaiter {}; }

        void return_value(T&& value) { m_data.emplace(util::forward<T>(value)); }

        void unhandled_exception() { util::unreachable(); }

    private:
        template<typename>
        friend class Lazy;

        struct FinalAwaiter {
            bool await_ready() noexcept { return false; }

            template<typename Promise>
            CoroutineHandle<> await_suspend(CoroutineHandle<Promise> coroutine) noexcept {
                PromiseBase& current = coroutine.promise();
                return current.continuation() ? current.continuation() : noop_coroutine();
            }

            void await_resume() noexcept {}
        };

        struct Awaiter {
            CoroutineHandle<PromiseBase> coroutine;

            bool await_ready() noexcept { return false; }

            template<typename OtherPromise>
            CoroutineHandle<> await_suspend(CoroutineHandle<OtherPromise> continuation) noexcept {
                // If we don't have a coroutine, it is because allocating it failed. Since the continuation is already
                // suspended, we can just report the error and return a noop coroutine.
                if (!coroutine) {
                    continuation.promise().unhandled_error(vocab::Error(BasicError::NotEnoughMemory));
                    return noop_coroutine();
                }
                coroutine.promise().set_continuation(continuation);
                return coroutine;
            }

            T await_resume() {
                DI_ASSERT(coroutine);
                auto& promise = static_cast<PromiseBase&>(coroutine.promise());
                DI_ASSERT(promise.m_data);
                return *util::move(promise.m_data);
            }
        };

        Optional<T> m_data;
    };

    template<typename Self>
    class PromiseBase<Self, void> : public WithAwaitableSenders<Self> {
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
                return current.continuation() ? current.continuation() : noop_coroutine();
            }

            void await_resume() noexcept {}
        };

        struct Awaiter {
            CoroutineHandle<PromiseBase> coroutine;

            bool await_ready() noexcept { return false; }

            template<typename OtherPromise>
            CoroutineHandle<> await_suspend(CoroutineHandle<OtherPromise> continuation) noexcept {
                // If we don't have a coroutine, it is because allocating it failed. Since the continuation is already
                // suspended, we can just report the error and return a noop coroutine.
                if (!coroutine) {
                    continuation.promise().unhandled_error(vocab::Error(BasicError::NotEnoughMemory));
                    return noop_coroutine();
                }
                coroutine.promise().set_continuation(continuation);
                return coroutine;
            }

            void await_resume() noexcept { DI_ASSERT(coroutine); }
        };

        CoroutineHandle<> m_continuation;
    };

    template<typename T>
    class [[nodiscard]] Lazy {
    private:
        struct Promise;

        using PromiseBase = lazy_ns::PromiseBase<Promise, T>;

        struct Promise : PromiseBase {
            Lazy get_return_object() noexcept { return Lazy { CoroutineHandle<Promise>::from_promise(*this) }; }
            static Lazy get_return_object_on_allocation_failure() noexcept { return Lazy { AllocFailed {} }; }
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
            if (!m_handle) {
                return Awaiter { nullptr };
            }
            auto& promise = static_cast<PromiseBase&>(m_handle.promise());
            return Awaiter { ParentHandle::from_promise(promise) };
        }

    private:
        explicit Lazy(Handle handle) : m_handle(handle) {}
        explicit Lazy(AllocFailed) {}

        Handle m_handle;
    };
}

using lazy_ns::Lazy;
}

namespace di::vocab {
template<concepts::Expected T, typename Promise>
constexpr decltype(auto) tag_invoke(types::Tag<execution::as_awaitable>, T&& value, Promise& promise) {
    return execution::as_awaitable(execution::just_or_error(util::forward<T>(value)), promise);
}

template<concepts::Unexpected T, typename Promise>
constexpr decltype(auto) tag_invoke(types::Tag<execution::as_awaitable>, T&& value, Promise& promise) {
    return execution::as_awaitable(execution::just_error(util::forward<T>(value).error()), promise);
}
}
