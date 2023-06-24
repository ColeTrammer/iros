#pragma once

#include <di/execution/concepts/sender_in.h>
#include <di/execution/meta/gather_signatures.h>
#include <di/execution/meta/variant_or_empty.h>
#include <di/execution/receiver/set_error.h>
#include <di/execution/types/empty_env.h>
#include <di/meta/util.h>

namespace di::meta {
template<typename Sender, typename Env = types::EmptyEnv, template<typename...> typename Var = meta::VariantOrEmpty>
requires(concepts::SenderIn<Sender, Env>)
using ErrorTypesOf = GatherSignatures<execution::SetError, Sender, Env, meta::TypeIdentity, Var>;
}
