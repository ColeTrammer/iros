#pragma once

#include <di/execution/concepts/is_awaiter.h>
#include <di/util/forward.h>
#include <di/util/voidify.h>

namespace di::concepts {
namespace detail {
    // This functions returns the value of applying co_await (awaitable), when called in a
    // coroutine a specified promise type. C++ first tries to call a member operator co_await,
    // then a global operator co_await, and finally returns the awaitable unmodified.
    template<typename Awaitable>
    decltype(auto) get_awaiter(Awaitable&& awaitable, void*) {
        if constexpr (requires { util::forward<Awaitable>(awaitable).operator co_await(); }) {
            return util::forward<Awaitable>(awaitable).operator co_await();
        } else if constexpr (requires { operator co_await(util::forward<Awaitable>(awaitable)); }) {
            return operator co_await(util::forward<Awaitable>(awaitable));
        } else {
            return util::forward<Awaitable>(awaitable);
        }
    }

    // When a concrete promise type is known, and it has a member await_transform(), C++
    // calls that function before applying the above rules.
    template<typename Awaitable, typename Promise>
    decltype(auto) get_awaiter(Awaitable&& awaitable, Promise* promise)
    requires(requires { promise->await_transform(util::forward<Awaitable>(awaitable)); })
    {
        return get_awaiter(promise->await_transform(util::forward<Awaitable>(awaitable)), util::voidify(promise));
    }
}

// An expression is awaitable given a promise type if it can be co_await'ed. This requires that the
// above detail::get_awaiter() function returns a valid Awaiter object.
template<typename T, typename Promise = void>
concept IsAwaitable = requires(T (&f)() noexcept, Promise* promise) {
    { detail::get_awaiter(f(), promise) } -> IsAwaiter<Promise>;
};
}
