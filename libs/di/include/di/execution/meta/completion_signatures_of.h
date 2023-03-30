#pragma once

#include <di/concepts/not_same_as.h>
#include <di/execution/query/get_completion_signatures.h>
#include <di/util/declval.h>

namespace di::meta {
template<typename Sender, typename Env = types::NoEnv>
requires(requires {
            {
                execution::get_completion_signatures(util::declval<Sender>(), util::declval<Env>())
            } -> concepts::NotSameAs<execution::detail::NoCompletionSignatures>;
        })
using CompletionSignaturesOf =
    decltype(execution::get_completion_signatures(util::declval<Sender>(), util::declval<Env>()));
}
