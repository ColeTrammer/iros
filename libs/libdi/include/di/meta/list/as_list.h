#pragma once

#include <di/meta/integer_sequence.h>
#include <di/meta/list/list.h>

namespace di::meta {
namespace detail {
    template<typename T>
    struct AsListHelper {};

    template<typename T, T... values>
    struct AsListHelper<IntegerSequence<T, values...>> : TypeConstant<List<IntegralConstant<T, values>...>> {};
}

template<typename T>
using AsList = detail::AsListHelper<T>::Type;
}