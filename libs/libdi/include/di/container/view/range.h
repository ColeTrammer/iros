#pragma once

#include <di/concepts/integer.h>
#include <di/concepts/signed_integer.h>
#include <di/container/view/range.h>

namespace di::container::view {
namespace detail {
    struct RangeFunction {
        template<concepts::Integer T>
        constexpr auto operator()(T end) const {
            return iota(static_cast<T>(0), end);
        }

        template<concepts::Integer T, concepts::Integer U>
        requires(concepts::SignedInteger<T> == concepts::SignedInteger<U>)
        constexpr auto operator()(T start, U end) const {
            return iota(start, end);
        }
    };
}

constexpr inline auto range = detail::RangeFunction {};
}
