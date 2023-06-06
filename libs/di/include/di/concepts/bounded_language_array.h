#pragma once

#include <di/types/prelude.h>

namespace di::concepts {
namespace detail {
    template<typename T>
    constexpr inline bool bounded_language_array_helper = false;

    template<typename T, usize N>
    constexpr inline bool bounded_language_array_helper<T[N]> = true;
}

template<typename T>
concept BoundedLanguageArray = detail::bounded_language_array_helper<T>;
}
