#pragma once

#include <di/meta/list/invoke.h>
#include <di/meta/list/list.h>
#include <di/meta/list/type.h>

namespace di::meta {
namespace detail {
    template<typename List, typename Acc, typename MetaFn>
    struct FoldHelper {};

    template<typename Acc, typename MetaFn>
    struct FoldHelper<List<>, Acc, MetaFn> : TypeConstant<Acc> {};

    template<typename T, typename... Rest, typename Acc, typename MetaFn>
    struct FoldHelper<List<T, Rest...>, Acc, MetaFn>
        : TypeConstant<Type<FoldHelper<List<Rest...>, Invoke<MetaFn, Acc, T>, MetaFn>>> {};
}

template<concepts::TypeList List, typename Init, concepts::MetaInvocable MetaFn>
using Fold = Type<detail::FoldHelper<List, Init, MetaFn>>;
}
