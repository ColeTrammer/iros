#pragma once

#include <di/meta/false_type.h>
#include <di/meta/true_type.h>

namespace di::concepts {
namespace detail {
    template<typename T>
    struct LValueReferenceHelper : meta::FalseType {};

    template<typename T>
    struct LValueReferenceHelper<T&> : meta::TrueType {};
}

template<typename T>
concept LValueReference = detail::LValueReferenceHelper<T>::value;
}
