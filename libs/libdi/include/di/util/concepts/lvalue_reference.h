#pragma once

#include <di/util/meta/false_type.h>
#include <di/util/meta/true_type.h>

namespace di::util::concepts {
namespace detail {
    template<typename T>
    struct LValueReferenceHelper : meta::FalseType {};

    template<typename T>
    struct LValueReferenceHelper<T&> : meta::TrueType {};
}

template<typename T>
concept LValueReference = detail::LValueReferenceHelper<T>::value;
}
