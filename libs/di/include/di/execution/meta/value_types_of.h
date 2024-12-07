#pragma once

#include "di/execution/concepts/sender_in.h"
#include "di/execution/meta/decayed_tuple.h"
#include "di/execution/meta/gather_signatures.h"
#include "di/execution/meta/variant_or_empty.h"
#include "di/execution/receiver/set_value.h"
#include "di/execution/types/empty_env.h"

namespace di::meta {
template<typename Sender, typename Env = types::EmptyEnv, template<typename...> typename Tup = meta::DecayedTuple,
         template<typename...> typename Var = meta::VariantOrEmpty>
requires(concepts::SenderIn<Sender, Env>)
using ValueTypesOf = GatherSignatures<execution::SetValue, Sender, Env, Tup, Var>;
}
