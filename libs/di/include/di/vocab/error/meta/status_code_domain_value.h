#pragma once

#include <di/meta/core.h>
#include <di/vocab/error/erased.h>

namespace di::meta {
namespace detail {
    template<typename T>
    struct StatusCodeDomainValueHelper : TypeConstant<typename T::Value> {};

    template<typename T>
    struct StatusCodeDomainValueHelper<vocab::Erased<T>> : TypeConstant<T> {};
}

template<typename T>
using StatusCodeDomainValue = detail::StatusCodeDomainValueHelper<T>::Type;
}
