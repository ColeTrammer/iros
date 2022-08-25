#pragma once

#include <di/util/concepts/reference_wrapper.h>
#include <di/util/meta/type_constant.h>

namespace di::util::meta {
namespace detail {
    template<typename T>
    struct UnwrapReferenceHelper : TypeConstant<T> {};

    template<concepts::ReferenceWrapper T>
    struct UnwrapReferenceHelper<T> : TypeConstant<typename T::Value&> {};
}

template<typename T>
using UnwrapRererence = detail::UnwrapReferenceHelper<T>::Type;
}
