#pragma once

#include <di/meta/false_type.h>
#include <di/meta/true_type.h>

namespace di::concepts {
namespace detail {
    template<typename T>
    struct RValueReferenceHelper : meta::FalseType {};

    template<typename T>
    struct RValueReferenceHelper<T&&> : meta::TrueType {};
}

template<typename T>
concept RValueReference = detail::RValueReferenceHelper<T>::value;
}
