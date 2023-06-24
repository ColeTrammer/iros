#pragma once

#include <di/any/concepts/interface.h>
#include <di/any/concepts/method_callable_with.h>

namespace di::concepts {
namespace detail {
    template<typename T, typename I>
    constexpr inline bool impl_helper = false;

    template<typename T, typename... Methods>
    constexpr inline bool impl_helper<T, meta::List<Methods...>> = (MethodCallableWith<meta::Type<Methods>, T> && ...);

}

template<typename T, typename Interface>
concept Impl = detail::impl_helper<T, Interface>;
}

namespace di {
using concepts::Impl;
}
