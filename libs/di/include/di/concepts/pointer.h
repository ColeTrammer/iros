#pragma once

#include <di/meta/remove_cv.h>

namespace di::concepts {
namespace detail {
    template<typename T>
    constexpr inline bool pointer_helper = false;

    template<typename T>
    constexpr inline bool pointer_helper<T*> = true;
}

template<typename T>
concept Pointer = detail::pointer_helper<meta::RemoveCV<T>>;
}
