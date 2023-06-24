#pragma once

#include <di/container/algorithm/min_max_result.h>
#include <di/container/algorithm/minmax_element.h>
#include <di/container/concepts/prelude.h>
#include <di/container/iterator/prelude.h>
#include <di/container/meta/prelude.h>

namespace di::container {
namespace detail {
    struct MinMaxFunction {
        template<typename T, typename Proj = function::Identity,
                 concepts::IndirectStrictWeakOrder<meta::Projected<T const*, Proj>> Comp = function::Compare>
        constexpr MinMaxResult<T const&> operator()(T const& a, T const& b, Comp comp = {}, Proj proj = {}) const {
            if (function::invoke(comp, function::invoke(proj, a), function::invoke(proj, b)) <= 0) {
                return { a, b };
            } else {
                return { b, a };
            }
        }

        template<concepts::Copyable T, typename Proj = function::Identity,
                 concepts::IndirectStrictWeakOrder<meta::Projected<T const*, Proj>> Comp = function::Compare>
        constexpr MinMaxResult<T> operator()(std::initializer_list<T> list, Comp comp = {}, Proj proj = {}) const {
            auto result = container::minmax_element(list, util::ref(comp), util::ref(proj));
            return { *result.min, *result.max };
        }

        template<concepts::InputContainer Con, typename Proj = function::Identity,
                 concepts::IndirectStrictWeakOrder<meta::Projected<meta::ContainerIterator<Con>, Proj>> Comp =
                     function::Compare>
        requires(concepts::IndirectlyCopyableStorable<meta::ContainerIterator<Con>, meta::ContainerValue<Con>*>)
        constexpr MinMaxResult<meta::ContainerValue<Con>> operator()(Con&& container, Comp comp = {},
                                                                     Proj proj = {}) const {
            auto result = container::minmax_element(container, util::ref(comp), util::ref(proj));
            return { util::move(*result.min), util::move(*result.max) };
        }
    };
}

constexpr inline auto minmax = detail::MinMaxFunction {};
}

namespace di {
using container::minmax;
}
