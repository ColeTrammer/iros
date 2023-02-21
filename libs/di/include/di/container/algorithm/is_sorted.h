#pragma once

#include <di/container/algorithm/is_sorted_until.h>
#include <di/container/concepts/prelude.h>
#include <di/container/meta/prelude.h>
#include <di/function/compare.h>
#include <di/function/identity.h>
#include <di/util/move.h>
#include <di/util/reference_wrapper.h>

namespace di::container {
namespace detail {
    struct IsSortedFunction {
        template<concepts::ForwardIterator Iter, concepts::SentinelFor<Iter> Sent, typename Proj = function::Identity,
                 concepts::IndirectStrictWeakOrder<meta::Projected<Iter, Proj>> Comp = function::Compare>
        constexpr bool operator()(Iter first, Sent last, Comp comp = {}, Proj proj = {}) const {
            return container::is_sorted_until(util::move(first), last, util::ref(comp), util::ref(proj)) == last;
        }

        template<concepts::ForwardContainer Con, typename Proj = function::Identity,
                 concepts::IndirectStrictWeakOrder<meta::Projected<meta::ContainerIterator<Con>, Proj>> Comp =
                     function::Compare>
        constexpr bool operator()(Con&& container, Comp comp = {}, Proj proj = {}) const {
            return (*this)(container::begin(container), container::end(container), util::ref(comp), util::ref(proj));
        }
    };
}

constexpr inline auto is_sorted = detail::IsSortedFunction {};
}
