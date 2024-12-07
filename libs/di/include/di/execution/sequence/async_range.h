#pragma once

#include "di/execution/coroutine/env_promise.h"
#include "di/execution/meta/await_result.h"
#include "di/execution/types/empty_env.h"
#include "di/function/invoke.h"
#include "di/meta/vocab.h"

namespace di::execution {
namespace next_ns {
    struct Function {
        template<typename AsyncRange>
        requires(concepts::TagInvocable<Function, AsyncRange&>)
        auto operator()(AsyncRange& async_range) const {
            static_assert(
                concepts::IsAwaitable<meta::TagInvokeResult<Function, AsyncRange&>, EnvPromise<types::EmptyEnv>>,
                "The return value of execution::next() must be awaitable with an awaitable.");
            return function::tag_invoke(*this, async_range);
        }
    };
}

/// @brief Get the next value of a async range in a coroutine.
///
/// @param async_range The lvalue async range to get the next value from.
///
/// @returns An awaitable which will return the next value of the sequence.
///
/// This function is used in conjunction with co_await to consume an async sequence in a coroutine.
/// For instance:
///
/// ```cpp
/// namespace ex = di::execution;
///
/// auto g() -> di::AsyncGenerator<int> {
///     co_yield 1;
///     co_yield 2;
///     co_yield 3;
///     co_return {};
/// }
///
/// auto f() -> di::Lazy<> {
///     auto sequence = co_await g();
///     while (di::concepts::Optional auto next = co_await ex::next(sequence)) {
///         dius::println("{}"_sv, *next);
///     }
/// }
/// ```
///
/// The key points are that the awaitable returned by execution::next() will return a vocab::Optional<T>, with the
/// null value present once the sequence has been exhausted. This allows the sequence to be consumed in a while loop.
constexpr inline auto next = next_ns::Function {};
}

namespace di::concepts {
template<typename T>
concept AsyncRange = concepts::Optional<
    meta::AwaitResult<meta::InvokeResult<decltype(execution::next), T&>, execution::EnvPromise<types::EmptyEnv>>>;

template<typename T>
concept AwaitableAsyncRange = AsyncRange<meta::AwaitResult<T, execution::EnvPromise<types::EmptyEnv>>>;
}
