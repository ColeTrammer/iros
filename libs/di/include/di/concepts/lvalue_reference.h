#pragma once

namespace di::concepts {
namespace detail {
    template<typename T>
    constexpr inline bool lvalue_reference_helper = false;

    template<typename T>
    constexpr inline bool lvalue_reference_helper<T&> = true;

}

template<typename T>
concept LValueReference = detail::lvalue_reference_helper<T>;
}
