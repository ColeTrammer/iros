#pragma once

#include <di/concepts/same_as.h>
#include <di/execution/concepts/sender.h>
#include <di/execution/meta/gather_signatures.h>
#include <di/execution/receiver/set_stopped.h>
#include <di/meta/list/list.h>

namespace di::meta {
template<typename Sender, typename Env = types::NoEnv>
requires(concepts::Sender<Sender, Env>)
constexpr inline bool sends_stopped =
    (!concepts::SameAs<List<>, GatherSignatures<execution::SetStopped, Sender, Env, List, List>>);
}
