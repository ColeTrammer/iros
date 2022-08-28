#pragma once

#include <di/meta/false_type.h>
#include <di/meta/true_type.h>

namespace di::concepts {
namespace detail {
    template<typename T>
    struct ConstHelper : meta::FalseType {};

    template<typename T>
    struct ConstHelper<T const> : meta::TrueType {};
}

template<typename T>
concept Const = detail::ConstHelper<T>::value;
}
