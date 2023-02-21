#pragma once

#include <di/meta/list/invoke.h>
#include <di/meta/list/list.h>
#include <di/meta/list/type.h>

namespace di::meta {
namespace detail {
    template<typename List, typename Init, typename MetaFn>
    struct FoldRightHelper {};

    template<typename Init, typename MetaFn>
    struct FoldRightHelper<List<>, Init, MetaFn> : TypeConstant<Init> {};

    template<typename T, typename... Rest, typename Init, typename MetaFn>
    struct FoldRightHelper<List<T, Rest...>, Init, MetaFn>
        : TypeConstant<Invoke<MetaFn, Type<FoldRightHelper<List<Rest...>, Init, MetaFn>>, T>> {};
}

template<concepts::TypeList List, typename Init, concepts::MetaInvocable MetaFn>
using FoldRight = Type<detail::FoldRightHelper<List, Init, MetaFn>>;
}
