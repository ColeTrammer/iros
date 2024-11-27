#pragma once

#include <di/util/reference_wrapper.h>

namespace di::util {
namespace detail {
    struct UnwrapReferenceFunction {
        template<typename T>
        constexpr auto operator()(T& value) const -> T& {
            return value;
        }

        template<typename T>
        constexpr auto operator()(ReferenceWrapper<T> value) const -> T& {
            return value.get();
        }
    };
}

constexpr inline auto unwrap_reference = detail::UnwrapReferenceFunction {};
}

namespace di {
using util::unwrap_reference;
}
