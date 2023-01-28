#pragma once

#include <di/execution/concepts/sender.h>
#include <di/execution/meta/completion_signatures_of.h>
#include <di/execution/meta/decayed_tuple.h>
#include <di/execution/meta/variant_or_empty.h>
#include <di/execution/receiver/set_value.h>
#include <di/execution/types/no_env.h>
#include <di/meta/list/prelude.h>

namespace di::meta {
namespace detail {
    template<typename Completions, typename Env, template<typename...> typename Tup>
    struct ValueTypesOfHelper;

    template<typename Env, template<typename...> typename Tup>
    struct ValueTypesOfHelper<types::CompletionSignatures<>, Env, Tup> : TypeConstant<List<>> {};

    template<typename T, typename... Rest, typename Env, template<typename...> typename Tup>
    struct ValueTypesOfHelper<types::CompletionSignatures<T, Rest...>, Env, Tup>
        : ValueTypesOfHelper<types::CompletionSignatures<Rest...>, Env, Tup> {};

    template<typename... Values, typename... Rest, typename Env, template<typename...> typename Tup>
    struct ValueTypesOfHelper<types::CompletionSignatures<execution::SetValue(Values...), Rest...>, Env, Tup>
        : TypeConstant<PushFront<typename ValueTypesOfHelper<types::CompletionSignatures<Rest...>, Env, Tup>::Type,
                                 Tup<Values...>>> {};
}

template<typename Sender, typename Env = types::NoEnv, template<typename...> typename Tup = meta::DecayedTuple,
         template<typename...> typename Var = meta::VariantOrEmpty>
requires(concepts::Sender<Sender, Env>)
using ValueTypesOf =
    AsTemplate<Var, typename detail::ValueTypesOfHelper<meta::CompletionSignaturesOf<Sender, Env>, Env, Tup>::Type>;
}