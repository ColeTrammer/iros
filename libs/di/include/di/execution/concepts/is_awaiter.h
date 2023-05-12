#pragma once

#include <di/concepts/boolean_testable.h>
#include <di/concepts/language_void.h>
#include <di/concepts/same_as.h>
#include <di/execution/concepts/await_suspend_result.h>

namespace di::concepts {
namespace detail {
    template<typename Promise, typename Awaiter>
    decltype(auto) do_await_suspend(Awaiter& awaiter) {
        if constexpr (!concepts::SameAs<Promise, void>) {
            return awaiter.await_suspend(std::coroutine_handle<Promise> {});
        }
    }
}

// An awaiter object requires 3 methods: await_ready(), await_suspend(), and await_resume().
// Because in reality, await_suspend() is only called with coroutine handles with concrete promise
// types, it cannot be checked when the promise type is unknown.
template<typename T, typename Promise = void>
concept IsAwaiter = requires(T& awaiter) {
    { awaiter.await_ready() } -> BooleanTestable;
    { detail::do_await_suspend<Promise>(awaiter) } -> AwaitSuspendResult;
    awaiter.await_resume();
};
}
