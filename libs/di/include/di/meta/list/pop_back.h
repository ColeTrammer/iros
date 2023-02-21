#pragma once

#include <di/meta/list/list.h>

namespace di::meta {
namespace detail {
    template<typename T>
    struct PopBackHelper : TypeConstant<List<>> {};

    template<typename T, typename... Rest>
    struct PopBackHelper<List<Rest..., T>> : TypeConstant<List<Rest...>> {};
}

template<concepts::TypeList L>
using PopBack = meta::Type<detail::PopBackHelper<L>>;
}
