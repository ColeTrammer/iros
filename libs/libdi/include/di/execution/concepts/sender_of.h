#pragma once

#include <di/execution/concepts/sender.h>
#include <di/execution/meta/value_types_of.h>
#include <di/execution/types/no_env.h>
#include <di/meta/list/prelude.h>
#include <di/meta/type_identity.h>

namespace di::concepts {
template<typename Send, typename Env, typename... Values>
concept SenderOf =
    Sender<Send, Env> && SameAs<meta::List<Values...>, meta::ValueTypesOf<Send, Env, meta::List, meta::TypeIdentity>>;
}