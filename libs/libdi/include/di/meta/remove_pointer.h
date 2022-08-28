#pragma once

#include <di/meta/type_constant.h>

namespace di::meta {
namespace detail {
    template<typename T>
    struct RemovePointerHelper : TypeConstant<T> {};

    template<typename T>
    struct RemovePointerHelper<T*> : TypeConstant<T> {};

    template<typename T>
    struct RemovePointerHelper<T* const> : TypeConstant<T> {};

    template<typename T>
    struct RemovePointerHelper<T* volatile> : TypeConstant<T> {};

    template<typename T>
    struct RemovePointerHelper<T* const volatile> : TypeConstant<T> {};
}

template<typename T>
using RemovePointer = detail::RemovePointerHelper<T>::Type;
}
