#pragma once

#include <di/concepts/reference_wrapper.h>
#include <di/meta/type_constant.h>

namespace di::meta {
namespace detail {
    template<typename T>
    struct UnwrapReferenceHelper : TypeConstant<T> {};

    template<concepts::ReferenceWrapper T>
    struct UnwrapReferenceHelper<T> : TypeConstant<typename T::Value&> {};
}

template<typename T>
using UnwrapReference = detail::UnwrapReferenceHelper<T>::Type;
}
