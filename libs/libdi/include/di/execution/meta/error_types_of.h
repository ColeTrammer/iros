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
    template<typename Completions, typename Env>
    struct ErrorTypesOfHelper;

    template<typename Env>
    struct ErrorTypesOfHelper<types::CompletionSignatures<>, Env> : TypeConstant<List<>> {};

    template<typename T, typename... Rest, typename Env>
    struct ErrorTypesOfHelper<types::CompletionSignatures<T, Rest...>, Env>
        : ErrorTypesOfHelper<types::CompletionSignatures<Rest...>, Env> {};

    template<typename Error, typename... Rest, typename Env>
    struct ErrorTypesOfHelper<types::CompletionSignatures<execution::SetError(Error), Rest...>, Env>
        : TypeConstant<PushFront<typename ErrorTypesOfHelper<types::CompletionSignatures<Rest...>, Env>::Type, Error>> {
    };
}

template<typename Sender, typename Env = types::NoEnv, template<typename...> typename Var = meta::VariantOrEmpty>
requires(concepts::Sender<Sender, Env>)
using ErrorTypesOf =
    AsTemplate<Var, typename detail::ErrorTypesOfHelper<meta::CompletionSignaturesOf<Sender, Env>, Env>::Type>;
}