#pragma once

#include <di/execution/coroutine/as_awaitable.h>
#include <di/meta/util.h>

namespace di::execution {
template<concepts::ClassType Promise>
class WithAwaitableSenders {
public:
    template<typename OtherPromise>
    requires(!concepts::LanguageVoid<OtherPromise>)
    void set_continuation(CoroutineHandle<OtherPromise> handle) {
        m_continuation = handle;

        if constexpr (requires(OtherPromise& other) {
                          { other.unhandled_stopped() } -> concepts::ConvertibleTo<CoroutineHandle<>>;
                      }) {
            m_stopped_handler = [](void* address) -> CoroutineHandle<> {
                return CoroutineHandle<OtherPromise>::from_address(address).promise().unhandled_stopped();
            };
        } else {
            m_stopped_handler = default_unhandled_stopped;
        }

        if constexpr (requires(OtherPromise& other, Error error) {
                          { other.unhandled_error(util::move(error)) } -> concepts::ConvertibleTo<CoroutineHandle<>>;
                      }) {
            m_error_handler = [](void* address, Error error) -> CoroutineHandle<> {
                return CoroutineHandle<OtherPromise>::from_address(address).promise().unhandled_error(
                    util::move(error));
            };
        } else {
            m_error_handler = default_unhandled_error;
        }
    }

    auto continuation() const -> CoroutineHandle<> { return m_continuation; }
    auto unhandled_stopped() -> CoroutineHandle<> { return m_stopped_handler(m_continuation.address()); }
    auto unhandled_error(Error error) -> CoroutineHandle<> {
        return m_error_handler(m_continuation.address(), util::move(error));
    }

    template<typename Value>
    auto await_transform(Value&& value) -> decltype(auto) {
        return as_awaitable(util::forward<Value>(value), static_cast<Promise&>(*this));
    }

private:
    [[noreturn]] static auto default_unhandled_stopped(void*) -> CoroutineHandle<> {
        DI_ASSERT(false);
        util::unreachable();
    }
    [[noreturn]] static auto default_unhandled_error(void*, Error) -> CoroutineHandle<> {
        DI_ASSERT(false);
        util::unreachable();
    }

    CoroutineHandle<> m_continuation {};
    CoroutineHandle<> (*m_stopped_handler)(void*) { &default_unhandled_stopped };
    CoroutineHandle<> (*m_error_handler)(void*, Error) { &default_unhandled_error };
};
}

namespace di {
using execution::WithAwaitableSenders;
}
