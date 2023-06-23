#pragma once

#include <di/meta/core.h>

namespace di::meta {
namespace detail {
    template<typename T>
    struct RemoveVolatile : TypeConstant<T> {
        using Type = T;
    };

    template<typename T>
    struct RemoveVolatile<T volatile> : TypeConstant<T> {
        using Type = T;
    };
}

template<typename T>
using RemoveVolatile = detail::RemoveVolatile<T>::Type;
}
