#pragma once

#include <di/util/meta/type_constant.h>

namespace di::util::meta {
namespace detail {
    template<typename T>
    struct RemoveReferenceHelper : TypeConstant<T> {};

    template<typename T>
    struct RemoveReferenceHelper<T&> : TypeConstant<T> {};

    template<typename T>
    struct RemoveReferenceHelper<T&&> : TypeConstant<T> {};
}

template<typename T>
using RemoveReference = detail::RemoveReferenceHelper<T>::Type;
}
