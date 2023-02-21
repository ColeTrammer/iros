#pragma once

#include <di/concepts/move_constructible.h>
#include <di/execution/concepts/valid_completion_signatures.h>
#include <di/execution/query/get_completion_signatures.h>

namespace di::concepts {
namespace detail {
    template<typename Sender, typename Env>
    concept SenderBase =
        requires(Sender&& sender, Env&& env) {
            {
                execution::get_completion_signatures(util::forward<Sender>(sender), util::forward<Env>(env))
                } -> ValidCompletionSignatures<Env>;
        };
}

template<typename Send, typename Env = types::NoEnv>
concept Sender = detail::SenderBase<Send, Env> && detail::SenderBase<Send, types::NoEnv> &&
                 concepts::MoveConstructible<meta::RemoveCVRef<Send>>;
}
