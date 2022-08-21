#pragma once

#include <di/util/meta/type_constant.h>

namespace di::util::meta {
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
