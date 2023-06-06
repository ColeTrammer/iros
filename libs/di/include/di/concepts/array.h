#pragma once

#include <di/meta/remove_cvref.h>
#include <di/types/prelude.h>

namespace di::vocab {
template<typename T, usize size>
struct Array;
}

namespace di::concepts {
namespace detail {
    template<typename T>
    constexpr inline bool array_helper = false;

    template<typename T, usize size>
    constexpr inline bool array_helper<vocab::Array<T, size>> = true;
}

template<typename T>
concept Array = detail::array_helper<meta::RemoveCVRef<T>>;
}
