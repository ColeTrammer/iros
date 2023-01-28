#pragma once

#include <di/meta/false_type.h>
#include <di/meta/true_type.h>

namespace di::concepts {
namespace detail {
    template<typename T>
    struct UnboundedLanguageArrayHelper : meta::FalseType {};

    template<typename T>
    struct UnboundedLanguageArrayHelper<T[]> : meta::TrueType {};
}

template<typename T>
concept UnboundedLanguageArray = detail::UnboundedLanguageArrayHelper<T>::value;
}
