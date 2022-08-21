#pragma once

#include <di/util/meta/type_constant.h>
#include <di/util/types.h>

namespace di::util::meta {
namespace detail {
    template<typename T>
    struct RemoveExtentHelper : TypeConstant<T> {};

    template<typename T>
    struct RemoveExtentHelper<T[]> : TypeConstant<T> {};

    template<typename T, size_t N>
    struct RemoveExtentHelper<T[N]> : TypeConstant<T> {};
}

template<typename T>
using RemoveExtent = detail::RemoveExtentHelper<T>::Type;
}
