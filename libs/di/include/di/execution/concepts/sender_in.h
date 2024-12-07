#pragma once

#include "di/execution/concepts/sender.h"
#include "di/execution/concepts/valid_completion_signatures.h"
#include "di/execution/query/get_completion_signatures.h"
#include "di/execution/types/empty_env.h"

namespace di::concepts {
template<typename S, typename E = types::EmptyEnv>
concept SenderIn = concepts::Sender<S> && requires(S&& sender, E&& env) {
    {
        execution::get_completion_signatures(util::forward<S>(sender), util::forward<E>(env))
    } -> concepts::ValidCompletionSignatures;
};
}

namespace di {
using concepts::SenderIn;
}
