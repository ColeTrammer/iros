#pragma once

namespace di::concepts {
namespace detail {
    template<typename T, typename U>
    constexpr inline auto same_as_helper = false;

    template<typename T>
    constexpr inline auto same_as_helper<T, T> = true;
}

template<typename T, typename U>
concept SameAs = detail::same_as_helper<T, U>;
}
