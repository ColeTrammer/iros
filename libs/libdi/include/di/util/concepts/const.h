#pragma once

#include <di/util/meta/false_type.h>
#include <di/util/meta/true_type.h>

namespace di::util::concepts {
namespace detail {
    template<typename T>
    struct ConstHelper : meta::FalseType {};

    template<typename T>
    struct ConstHelper<T const> : meta::TrueType {};
}

template<typename T>
concept Const = detail::ConstHelper<T>::value;
}
