#pragma once

#include <di/meta/core.h>
#include <di/meta/remove_cv.h>

namespace di::concepts {
namespace detail {
    template<typename From, typename To>
    constexpr inline bool qualification_convertible_to = SameAs<From, To>;

    template<typename From, typename To>
    constexpr inline bool qualification_convertible_to<From, To const> = SameAs<meta::RemoveConst<From>, To>;

    template<typename From, typename To>
    constexpr inline bool qualification_convertible_to<From, To volatile> = SameAs<meta::RemoveVolatile<From>, To>;

    template<typename From, typename To>
    constexpr inline bool qualification_convertible_to<From, To const volatile> = SameAs<meta::RemoveCV<From>, To>;
}

template<typename From, typename To>
concept QualificationConvertibleTo = detail::qualification_convertible_to<From, To>;
}
