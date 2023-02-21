#pragma once

#include <di/meta/decay.h>
#include <di/meta/false_type.h>
#include <di/meta/true_type.h>
#include <di/vocab/error/status_code_forward_declaration.h>

namespace di::concepts {
namespace detail {
    template<typename T>
    struct StatusCodeHelper : meta::FalseType {};

    template<typename T>
    struct StatusCodeHelper<vocab::StatusCode<T>> : meta::TrueType {};
}

template<typename T>
concept StatusCode = detail::StatusCodeHelper<meta::Decay<T>>::value;
}
