#pragma once

#include <di/meta/list/list.h>

namespace di::meta {
namespace detail {
    template<typename T>
    struct PopFrontHelper : TypeConstant<List<>> {};

    template<typename T, typename... Rest>
    struct PopFrontHelper<List<T, Rest...>> : TypeConstant<List<Rest...>> {};
}

template<concepts::TypeList L>
using PopFront = meta::Type<detail::PopFrontHelper<L>>;
}