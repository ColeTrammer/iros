#pragma once

#include <di/meta/remove_cv.h>

namespace di::concepts {
namespace detail {

    template<typename T>
    constexpr inline bool member_pointer_helper = false;

    template<typename T, typename U>
    constexpr inline bool member_pointer_helper<T U::*> = true;
}

template<typename T>
concept MemberPointer = detail::member_pointer_helper<meta::RemoveCV<T>>;
}
