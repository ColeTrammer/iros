#pragma once

#include <di/meta/core.h>
#include <di/types/size_t.h>

namespace di::meta {
namespace detail {
    template<typename T>
    struct RemoveExtentHelper : TypeConstant<T> {};

    template<typename T>
    struct RemoveExtentHelper<T[]> : TypeConstant<T> {};

    template<typename T, types::size_t N>
    struct RemoveExtentHelper<T[N]> : TypeConstant<T> {};
}

template<typename T>
using RemoveExtent = detail::RemoveExtentHelper<T>::Type;
}
