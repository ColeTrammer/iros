#pragma once

#include <di/meta/list/list.h>
#include <di/meta/list/push_front.h>

namespace di::meta {
namespace detail {
    template<typename T>
    struct PopBackHelper : TypeConstant<List<>> {};

    template<typename T, typename U, typename... Rest>
    struct PopBackHelper<List<T, U, Rest...>>
        : TypeConstant<PushFront<typename PopBackHelper<List<U, Rest...>>::Type, T>> {};
}

template<concepts::TypeList L>
using PopBack = meta::Type<detail::PopBackHelper<L>>;
}
