#pragma once

#include "di/meta/language.h"

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

namespace di {
using util::to_underlying;
}
