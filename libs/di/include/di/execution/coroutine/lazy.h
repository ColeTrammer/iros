#pragma once

#include <di/assert/assert_bool.h>
#include <di/execution/algorithm/just.h>
#include <di/execution/algorithm/just_or_error.h>
#include <di/execution/coroutine/as_awaitable.h>
#include <di/execution/coroutine/with_await_transform.h>
#include <di/execution/coroutine/with_awaitable_senders.h>
#include <di/execution/types/prelude.h>
#include <di/meta/core.h>
#include <di/meta/language.h>
#include <di/meta/operations.h>
#include <di/meta/vocab.h>
#include <di/platform/prelude.h>
#include <di/types/void.h>
#include <di/util/coroutine.h>
#include <di/util/exchange.h>
#include <di/util/unreachable.h>
#include <di/vocab/error/error.h>
#include <di/vocab/error/prelude.h>
#include <di/vocab/error/result.h>
#include <di/vocab/expected/unexpect.h>
#include <di/vocab/expected/unexpected.h>
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

        auto operator new(usize size) noexcept -> void* { return ::operator new(size, std::nothrow); }
        void operator delete(void* ptr, usize size) noexcept { ::operator delete(ptr, size); }

        auto initial_suspend() noexcept -> SuspendAlways { return {}; }
        auto final_suspend() noexcept { return FinalAwaiter {}; }

        template<concepts::ConvertibleTo<T> U>
        void return_value(U&& value) {
            m_data.emplace(util::forward<U>(value));
        }

        void return_value(types::Void)
        requires(concepts::LanguageVoid<T>)
        {
            m_data.emplace();
        }

        template<typename E>
        requires(concepts::ConstructibleFrom<vocab::Error, E>)
        void return_value(vocab::Unexpected<E>&& error) {
            m_data = util::move(error);
        }

        void return_value(Stopped) { m_data = vocab::Unexpected(BasicError::OperationCanceled); }

        void unhandled_exception() { util::unreachable(); }

    private:
        template<typename>
        friend class Lazy;

        struct FinalAwaiter {
            auto await_ready() noexcept -> bool { return false; }

            template<typename Promise>
            auto await_suspend(CoroutineHandle<Promise> coroutine) noexcept -> CoroutineHandle<> {
                PromiseBase& current = coroutine.promise();

                auto was_error = !current.m_data.has_value();
                if (was_error) {
                    if (current.m_data == vocab::Unexpected(BasicError::OperationCanceled)) {
                        return current.unhandled_stopped();
                    }
                    return current.unhandled_error(util::move(current.m_data).error());
                }

                return current.continuation() ? current.continuation() : noop_coroutine();
            }

            void await_resume() noexcept {}
        };

        struct Awaiter {
            CoroutineHandle<PromiseBase> coroutine;

            auto await_ready() noexcept -> bool { return false; }

            template<typename OtherPromise>
            auto await_suspend(CoroutineHandle<OtherPromise> continuation) noexcept -> CoroutineHandle<> {
                // If we don't have a coroutine, it is because allocating it failed. Since the continuation is already
                // suspended, we can just report the error.
                if (!coroutine) {
                    return continuation.promise().unhandled_error(vocab::Error(BasicError::NotEnoughMemory));
                }

                // Otherwise, we can just resume the coroutine after setting the continuation.
                coroutine.promise().set_continuation(continuation);
                return coroutine;
            }

            auto await_resume() -> T {
                DI_ASSERT(coroutine);
                auto& promise = static_cast<PromiseBase&>(coroutine.promise());
                DI_ASSERT(promise.m_data);
                return *util::move(promise.m_data);
            }
        };

        vocab::Result<T> m_data { vocab::Unexpected(BasicError::OperationCanceled) };
    };

    template<typename T>
    class [[nodiscard]] Lazy {
    private:
        struct Promise;

        using PromiseBase = lazy_ns::PromiseBase<Promise, T>;

        struct Promise : PromiseBase {
            auto get_return_object() noexcept -> Lazy { return Lazy { CoroutineHandle<Promise>::from_promise(*this) }; }
            static auto get_return_object_on_allocation_failure() noexcept -> Lazy { return Lazy { AllocFailed {} }; }
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

        auto operator co_await() -> Awaiter {
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
constexpr auto tag_invoke(types::Tag<execution::as_awaitable>, T&& value, Promise& promise) -> decltype(auto) {
    return execution::as_awaitable(execution::just_or_error(util::forward<T>(value)), promise);
}

template<concepts::Unexpected T, typename Promise>
constexpr auto tag_invoke(types::Tag<execution::as_awaitable>, T&& value, Promise& promise) -> decltype(auto) {
    return execution::as_awaitable(execution::just_error(util::forward<T>(value).error()), promise);
}
}

namespace di {
using execution::Lazy;
}
