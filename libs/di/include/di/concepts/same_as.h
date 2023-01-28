#pragma once

#include <di/meta/false_type.h>
#include <di/meta/true_type.h>

namespace di::concepts {
namespace detail {
    template<typename T, typename U>
    struct SameAsHelper : meta::FalseType {};

    template<typename T>
    struct SameAsHelper<T, T> : meta::TrueType {};
}

template<typename T, typename U>
concept SameAs = detail::SameAsHelper<T, U>::value;
}
