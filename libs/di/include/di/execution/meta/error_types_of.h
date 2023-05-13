#pragma once

#include <di/execution/meta/gather_signatures.h>
#include <di/execution/meta/variant_or_empty.h>
#include <di/execution/receiver/set_error.h>
#include <di/execution/types/no_env.h>
#include <di/meta/type_identity.h>

namespace di::meta {
template<typename Sender, typename Env = types::NoEnv, template<typename...> typename Var = meta::VariantOrEmpty>
requires(concepts::Sender<Sender, Env>)
using ErrorTypesOf = GatherSignatures<execution::SetError, Sender, Env, meta::TypeIdentity, Var>;
}
