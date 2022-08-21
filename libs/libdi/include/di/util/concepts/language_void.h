#pragma once

#include <di/util/meta/false_type.h>
#include <di/util/meta/remove_cv.h>
#include <di/util/meta/true_type.h>

namespace di::util::concepts {
namespace detail {
    template<typename T>
    struct LanguageVoidHelper : meta::FalseType {};

    template<>
    struct LanguageVoidHelper<void> : meta::TrueType {};
}

template<typename T>
concept LanguageVoid = detail::LanguageVoidHelper<meta::RemoveCV<T>>::value;
}
