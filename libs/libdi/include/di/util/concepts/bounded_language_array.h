#pragma once

#include <di/util/meta/false_type.h>
#include <di/util/meta/true_type.h>
#include <di/util/types.h>

namespace di::util::concepts {
namespace detail {
    template<typename T>
    struct BoundedLanguageArrayHelper : meta::FalseType {};

    template<typename T, size_t N>
    struct BoundedLanguageArrayHelper<T[N]> : meta::TrueType {};
}

template<typename T>
concept BoundedLanguageArray = detail::BoundedLanguageArrayHelper<T>::value;
}
