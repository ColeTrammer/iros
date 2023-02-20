#pragma once

#include <di/util/forward.h>
#include <di/util/std_new.h>

#ifdef DI_USE_STD
#include <memory>
#else
namespace std {
template<typename T, typename... Args>
constexpr T* construct_at(T* location, Args&&... args) {
    return ::new (const_cast<void*>(static_cast<const volatile void*>(location))) T(di::util::forward<Args>(args)...);
}
}
#endif

namespace di::util {
namespace detail {
    struct ConstructAtFunction {
        template<typename T, typename... Args>
        constexpr T* operator()(T* location, Args&&... args) const
        requires(requires { std::construct_at(location, util::forward<Args>(args)...); })
        {
            return std::construct_at(location, util::forward<Args>(args)...);
        }
    };
}

constexpr inline auto construct_at = detail::ConstructAtFunction {};
}
