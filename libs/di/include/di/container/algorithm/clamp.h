#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/meta/prelude.h>
#include <di/function/compare.h>
#include <di/function/identity.h>

namespace di::container {
namespace detail {
    struct ClampFunction {
        template<typename T, typename Proj = function::Identity,
                 concepts::IndirectStrictWeakOrder<meta::Projected<T const*, Proj>> Comp = function::Compare>
        constexpr T const& operator()(T const& value, T const& min, T const& max, Comp comp = {},
                                      Proj proj = {}) const {
            auto&& projected_value = function::invoke(proj, value);
            if (function::invoke(comp, projected_value, function::invoke(proj, min)) < 0) {
                return min;
            }
            if (function::invoke(comp, projected_value, function::invoke(proj, max)) > 0) {
                return max;
            }
            return value;
        }
    };
}

constexpr inline auto clamp = detail::ClampFunction {};
}