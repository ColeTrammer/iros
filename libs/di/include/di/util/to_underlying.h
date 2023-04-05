#pragma once

#include <di/concepts/enum.h>
#include <di/meta/underlying_type.h>

namespace di::util {
namespace detail {
    struct ToUnderlyingFunction {
        template<concepts::Enum T>
        constexpr auto operator()(T value) const {
            return static_cast<meta::UnderlyingType<T>>(value);
        }
    };
}

constexpr inline auto to_underlying = detail::ToUnderlyingFunction {};
}
