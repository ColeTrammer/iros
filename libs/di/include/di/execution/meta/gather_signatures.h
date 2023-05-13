#pragma once

#include <di/concepts/always_true.h>
#include <di/concepts/same_as.h>
#include <di/execution/concepts/completion_signature.h>
#include <di/execution/concepts/sender_in.h>
#include <di/execution/meta/completion_signatures_of.h>
#include <di/meta/language_function_return.h>
#include <di/meta/list/as_list.h>
#include <di/meta/list/type.h>
#include <di/meta/type_constant.h>

namespace di::meta {
namespace detail {
    template<bool>
    struct IndirectMetaApply {
        template<template<typename...> typename T, typename... Args>
        using MetaApply = T<Args...>;
    };

    template<template<typename...> typename F, typename List>
    struct MetaApply;

    template<template<typename...> typename F, typename... Args>
    struct MetaApply<F, meta::List<Args...>>
        : TypeConstant<typename IndirectMetaApply<concepts::AlwaysTrue<Args...>>::template MetaApply<F, Args...>> {};

    template<typename Tag, typename Completions, template<typename...> typename Tup>
    struct GatherSignaturesHelper;

    template<typename Tag, template<typename...> typename Tup>
    struct GatherSignaturesHelper<Tag, List<>, Tup> : TypeConstant<List<>> {};

    template<typename Tag, typename T, typename... Rest, template<typename...> typename Tup>
    struct GatherSignaturesHelper<Tag, List<T, Rest...>, Tup> : GatherSignaturesHelper<Tag, List<Rest...>, Tup> {};

    template<typename Tag, concepts::CompletionSignature T, typename... Rest, template<typename...> typename Tup>
    requires(concepts::SameAs<meta::LanguageFunctionReturn<T>, Tag>)
    struct GatherSignaturesHelper<Tag, List<T, Rest...>, Tup>
        : TypeConstant<PushFront<Type<GatherSignaturesHelper<Tag, List<Rest...>, Tup>>,
                                 meta::Type<MetaApply<Tup, meta::AsList<T>>>>> {};
}

template<typename Tag, typename S, typename E, template<typename...> typename Tuple,
         template<typename...> typename Variant>
requires(concepts::SenderIn<S, E>)
using GatherSignatures = meta::Type<detail::MetaApply<
    Variant, meta::Type<detail::GatherSignaturesHelper<Tag, meta::AsList<meta::CompletionSignaturesOf<S, E>>, Tuple>>>>;
}
