#pragma once

#include <di/assert/prelude.h>
#include <di/concepts/copyable.h>
#include <di/container/algorithm/max_element.h>
#include <di/container/concepts/indirect_strict_weak_order.h>
#include <di/container/concepts/prelude.h>
#include <di/container/meta/prelude.h>
#include <di/function/compare.h>
#include <di/function/identity.h>
#include <di/function/invoke.h>
#include <di/util/initializer_list.h>
#include <di/util/reference_wrapper.h>

namespace di::container {
namespace detail {
    struct MaxFunction {
        template<typename T, typename Proj = function::Identity,
                 concepts::IndirectStrictWeakOrder<meta::Projected<T const*, Proj>> Comp = function::Compare>
        constexpr T const& operator()(T const& a, T const& b, Comp comp = {}, Proj proj = {}) const {
            return function::invoke(comp, function::invoke(proj, a), function::invoke(proj, b)) >= 0 ? a : b;
        }

        template<concepts::Copyable T, typename Proj = function::Identity,
                 concepts::IndirectStrictWeakOrder<meta::Projected<T const*, Proj>> Comp = function::Compare>
        constexpr T const operator()(std::initializer_list<T> list, Comp comp = {}, Proj proj = {}) const {
            return *max_element(list, util::ref(comp), util::ref(proj));
        }

        template<concepts::InputContainer Con, typename Proj = function::Identity,
                 concepts::IndirectStrictWeakOrder<meta::Projected<meta::ContainerIterator<Con>, Proj>> Comp =
                     function::Compare>
        requires(concepts::IndirectlyCopyableStorable<meta::ContainerIterator<Con>, meta::ContainerValue<Con>*>)
        constexpr meta::ContainerValue<Con> operator()(Con&& container, Comp comp = {}, Proj proj = {}) const {
            auto it = container::begin(container);
            auto ed = container::end(container);
            auto result = meta::ContainerValue<Con> { *it };
            while (++it != ed) {
                if (function::invoke(comp, function::invoke(proj, *it), function::invoke(proj, result)) > 0) {
                    result = *it;
                }
            }
            return result;
        }
    };
}

constexpr inline auto max = detail::MaxFunction {};
}
