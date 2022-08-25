#pragma once

#include <di/util/concepts/lvalue_reference.h>
#include <di/util/meta/remove_reference.h>
#include <di/util/meta/type_constant.h>
#include <di/util/reference_wrapper.h>

namespace di::util::meta {
namespace detail {
    template<typename T>
    struct WrapReference : TypeConstant<T> {};

    template<concepts::LValueReference T>
    struct WrapReference<T> : TypeConstant<ReferenceWrapper<RemoveReference<T>>> {};
}

template<typename T>
using WrapReference = detail::WrapReference<T>::Type;
}
