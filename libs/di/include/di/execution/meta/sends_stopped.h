#pragma once

#include <di/execution/concepts/sender_in.h>
#include <di/execution/meta/gather_signatures.h>
#include <di/execution/receiver/set_stopped.h>
#include <di/execution/types/empty_env.h>
#include <di/meta/algorithm.h>
#include <di/meta/core.h>

namespace di::meta {
template<typename Sender, typename Env = types::EmptyEnv>
requires(concepts::SenderIn<Sender, Env>)
constexpr inline bool sends_stopped =
    (!concepts::SameAs<List<>, GatherSignatures<execution::SetStopped, Sender, Env, List, List>>);
}
