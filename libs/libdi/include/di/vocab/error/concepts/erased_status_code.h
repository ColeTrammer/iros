#pragma once

#include <di/meta/decay.h>
#include <di/meta/false_type.h>
#include <di/meta/true_type.h>
#include <di/vocab/error/erased.h>
#include <di/vocab/error/status_code_forward_declaration.h>

namespace di::concepts {
namespace detail {
    template<typename T>
    struct ErasedStatusCodeHelper : meta::FalseType {};

    template<typename T>
    struct ErasedStatusCodeHelper<vocab::StatusCode<vocab::Erased<T>>> : meta::TrueType {};
}

template<typename T>
concept ErasedStatusCode = detail::ErasedStatusCodeHelper<meta::Decay<T>>::value;
}