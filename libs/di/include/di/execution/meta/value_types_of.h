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
    template<typename Completions, template<typename...> typename Tup>
    struct ValueTypesOfHelper;

    template<template<typename...> typename Tup>
    struct ValueTypesOfHelper<List<>, Tup> : TypeConstant<List<>> {};

    template<typename T, typename... Rest, template<typename...> typename Tup>
    struct ValueTypesOfHelper<List<T, Rest...>, Tup> : ValueTypesOfHelper<List<Rest...>, Tup> {};

    template<concepts::LanguageFunction T, typename... Rest, template<typename...> typename Tup>
    requires(concepts::SameAs<meta::LanguageFunctionReturn<T>, execution::SetValue>)
    struct ValueTypesOfHelper<List<T, Rest...>, Tup>
        : TypeConstant<
              PushFront<Type<ValueTypesOfHelper<List<Rest...>, Tup>>, meta::Apply<meta::Quote<Tup>, meta::AsList<T>>>> {
    };
}

template<typename Sender, typename Env = types::NoEnv, template<typename...> typename Tup = meta::DecayedTuple,
         template<typename...> typename Var = meta::VariantOrEmpty>
requires(concepts::Sender<Sender, Env> &&
         requires {
             typename AsTemplate<
                 Var, Type<detail::ValueTypesOfHelper<meta::AsList<meta::CompletionSignaturesOf<Sender, Env>>, Tup>>>;
         })
using ValueTypesOf =
    AsTemplate<Var, Type<detail::ValueTypesOfHelper<meta::AsList<meta::CompletionSignaturesOf<Sender, Env>>, Tup>>>;
}
