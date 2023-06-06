#pragma once

#include <di/meta/remove_cv.h>

namespace di::concepts {
namespace detail {
    template<typename T>
    constexpr inline bool language_void_helper = false;

    template<>
    constexpr inline bool language_void_helper<void> = true;
}

template<typename T>
concept LanguageVoid = detail::language_void_helper<meta::RemoveCV<T>>;
}
