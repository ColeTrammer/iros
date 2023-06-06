#pragma once

namespace di::concepts {
namespace detail {
    template<typename T>
    constexpr inline bool rvalue_reference_helper = false;

    template<typename T>
    constexpr inline bool rvalue_reference_helper<T&&> = true;
}

template<typename T>
concept RValueReference = detail::rvalue_reference_helper<T>;
}
