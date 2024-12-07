#pragma once

#include "di/execution/concepts/sender_in.h"
#include "di/execution/query/get_completion_signatures.h"
#include "di/execution/types/empty_env.h"
#include "di/meta/util.h"
#include "di/util/declval.h"

namespace di::meta {
template<typename Sender, typename Env = types::EmptyEnv>
requires(concepts::SenderIn<Sender, Env>)
using CompletionSignaturesOf =
    decltype(execution::get_completion_signatures(util::declval<Sender>(), util::declval<Env>()));
}
