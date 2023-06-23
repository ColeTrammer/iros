#pragma once

#include <di/concepts/lvalue_reference.h>
#include <di/meta/core.h>
#include <di/meta/remove_reference.h>
#include <di/util/reference_wrapper.h>

namespace di::meta {
namespace detail {
    template<typename T>
    struct WrapReference : TypeConstant<T> {};

    template<concepts::LValueReference T>
    struct WrapReference<T> : TypeConstant<util::ReferenceWrapper<RemoveReference<T>>> {};
}

template<typename T>
using WrapReference = detail::WrapReference<T>::Type;
}
