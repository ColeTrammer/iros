#pragma once

#include <di/meta/type_constant.h>
#include <di/vocab/error/erased.h>
#include <di/vocab/error/status_code_forward_declaration.h>

namespace di::meta {
namespace detail {
    template<typename T>
    struct StatusCodeDomainHelper : TypeConstant<T> {};

    template<typename T>
    struct StatusCodeDomainHelper<vocab::Erased<T>> : TypeConstant<vocab::StatusCodeDomain> {};
}

template<typename T>
using StatusCodeDomain = detail::StatusCodeDomainHelper<T>::Type;
}