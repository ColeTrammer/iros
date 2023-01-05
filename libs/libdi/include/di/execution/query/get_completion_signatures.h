#pragma once

#include <di/concepts/instance_of.h>
#include <di/execution/types/completion_signuatures.h>
#include <di/execution/types/dependent_completion_signatures.h>
#include <di/execution/types/no_env.h>
#include <di/function/tag_invoke.h>

namespace di::execution {
namespace detail {
    struct NoCompletionSignatures {};

    struct GetCompletionSignaturesFunction {
        template<typename Sender>
        constexpr auto operator()(Sender&& sender) const {
            return (*this)(util::forward<Sender>(sender), types::NoEnv {});
        }

        template<typename Sender, typename Env>
        constexpr auto operator()(Sender&&, Env&&) const {
            if constexpr (requires { meta::TagInvokeResult<GetCompletionSignaturesFunction, Sender, Env> {}; }) {
                using Result = meta::TagInvokeResult<GetCompletionSignaturesFunction, Sender, Env>;
                static_assert(concepts::InstanceOf<Result, types::CompletionSignatures> ||
                                  concepts::SameAs<Result, types::DependentCompletionSignatures<Env>>,
                              "A customized get_completion_signatures() must return an instance of di::CompletionSignatures.");
                return Result {};
            } else if constexpr (requires { typename meta::RemoveCVRef<Sender>::CompletionSignatures; }) {
                using Result = meta::RemoveCVRef<Sender>::CompletionSignatures;
                static_assert(concepts::InstanceOf<Result, types::CompletionSignatures> ||
                                  concepts::SameAs<Result, types::DependentCompletionSignatures<Env>>,
                              "A sender's CompletionSignatures typedef must be an instance of di::CompletionSignatures.");
                return Result {};
            } else {
                // TODO: also handle completion signatures for awaitables.
                return NoCompletionSignatures {};
            }
        }
    };
}

constexpr inline auto get_completion_signatures = detail::GetCompletionSignaturesFunction {};
}