#pragma once

#include <di/meta/false_type.h>
#include <di/meta/true_type.h>
#include <di/types/size_t.h>

namespace di::concepts {
namespace detail {
    template<typename T>
    struct BoundedLanguageArrayHelper : meta::FalseType {};

    template<typename T, types::size_t N>
    struct BoundedLanguageArrayHelper<T[N]> : meta::TrueType {};
}

template<typename T>
concept BoundedLanguageArray = detail::BoundedLanguageArrayHelper<T>::value;
}
