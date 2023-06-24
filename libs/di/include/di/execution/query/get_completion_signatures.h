#pragma once

#include <di/execution/concepts/is_awaitable.h>
#include <di/execution/coroutine/env_promise.h>
#include <di/execution/meta/await_result.h>
#include <di/execution/types/completion_signuatures.h>
#include <di/function/tag_invoke.h>
#include <di/meta/core.h>
#include <di/meta/util.h>
#include <di/vocab/error/prelude.h>

namespace di::execution {
namespace detail {
    struct GetCompletionSignaturesFunction {
        template<typename Sender, typename Env>
        requires(concepts::TagInvocable<GetCompletionSignaturesFunction, Sender, Env> ||
                 requires { typename meta::RemoveCVRef<Sender>::CompletionSignatures; } ||
                 concepts::IsAwaitable<Sender, EnvPromise<Env>>)
        constexpr auto operator()(Sender&&, Env&&) const {
            if constexpr (requires { meta::TagInvokeResult<GetCompletionSignaturesFunction, Sender, Env> {}; }) {
                using Result = meta::TagInvokeResult<GetCompletionSignaturesFunction, Sender, Env>;
                static_assert(
                    concepts::InstanceOf<Result, types::CompletionSignatures>,
                    "A customized get_completion_signatures() must return an instance of di::CompletionSignatures.");
                return Result {};
            } else if constexpr (requires { typename meta::RemoveCVRef<Sender>::CompletionSignatures; }) {
                using Result = meta::RemoveCVRef<Sender>::CompletionSignatures;
                static_assert(
                    concepts::InstanceOf<Result, types::CompletionSignatures>,
                    "A sender's CompletionSignatures typedef must be an instance of di::CompletionSignatures.");
                return Result {};
            } else if constexpr (concepts::IsAwaitable<Sender, EnvPromise<Env>>) {
                if constexpr (concepts::LanguageVoid<meta::AwaitResult<Sender, EnvPromise<Env>>>) {
                    return types::CompletionSignatures<SetValue(), SetError(Error), SetStopped()> {};
                } else {
                    return types::CompletionSignatures<SetValue(meta::AwaitResult<Sender, EnvPromise<Env>>),
                                                       SetError(Error), SetStopped()> {};
                }
            }
        }
    };
}

constexpr inline auto get_completion_signatures = detail::GetCompletionSignaturesFunction {};
}
