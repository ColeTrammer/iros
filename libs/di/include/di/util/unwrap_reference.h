#pragma once

#include <di/util/reference_wrapper.h>

namespace di::util {
namespace detail {
    struct UnwrapReferenceFunction {
        template<typename T>
        constexpr T& operator()(T& value) const {
            return value;
        }

        template<typename T>
        constexpr T& operator()(ReferenceWrapper<T> value) const {
            return value.get();
        }
    };
}

constexpr inline auto unwrap_reference = detail::UnwrapReferenceFunction {};
}

namespace di {
using util::unwrap_reference;
}
