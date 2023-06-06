#pragma once

namespace di::concepts {
namespace detail {
    template<typename T>
    constexpr inline bool unbounded_language_array_helper = false;

    template<typename T>
    constexpr inline bool unbounded_language_array_helper<T[]> = true;
}

template<typename T>
concept UnboundedLanguageArray = detail::unbounded_language_array_helper<T>;
}
