#pragma once

#include <di/meta/list/list.h>

namespace di::meta {
namespace detail {
    template<typename T, size_t N>
    struct RepeatHelper;

    template<typename T>
    struct RepeatHelper<T, 0> : TypeConstant<List<>> {};

    template<typename T>
    struct RepeatHelper<T, 1> : TypeConstant<List<T>> {};

    template<typename T, size_t N>
    requires(N > 1)
    struct RepeatHelper<T, N>
        : TypeConstant<Concat<typename RepeatHelper<T, N / 2>::Type, typename RepeatHelper<T, (N + 1) / 2>::Type>> {};
}

template<typename T, size_t N>
using Repeat = detail::RepeatHelper<T, N>::Type;
}
