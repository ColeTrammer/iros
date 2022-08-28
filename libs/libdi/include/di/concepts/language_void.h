#pragma once

#include <di/meta/false_type.h>
#include <di/meta/remove_cv.h>
#include <di/meta/true_type.h>

namespace di::concepts {
namespace detail {
    template<typename T>
    struct LanguageVoidHelper : meta::FalseType {};

    template<>
    struct LanguageVoidHelper<void> : meta::TrueType {};
}

template<typename T>
concept LanguageVoid = detail::LanguageVoidHelper<meta::RemoveCV<T>>::value;
}
