#pragma once

#include <di/container/algorithm/lower_bound.h>
#include <di/container/algorithm/upper_bound.h>

namespace di::container {
namespace detail {
    struct EqualRangeFunction {
        template<concepts::ForwardIterator It, concepts::SentinelFor<It> Sent, typename T,
                 typename Proj = function::Identity,
                 concepts::IndirectStrictWeakOrder<T const*, meta::Projected<It, Proj>> Comp = function::Compare>
        constexpr View<It> operator()(It first, Sent last, T const& needle, Comp comp = {}, Proj proj = {}) const {
            auto const distance = container::distance(first, last);
            return equal_range_with_size(util::move(first), needle, util::ref(comp), util::ref(proj), distance);
        }

        template<concepts::ForwardContainer Con, typename T, typename Proj = function::Identity,
                 concepts::IndirectStrictWeakOrder<T const*, meta::Projected<meta::ContainerIterator<Con>, Proj>> Comp =
                     function::Compare>
        constexpr meta::BorrowedView<Con> operator()(Con&& container, T const& needle, Comp comp = {},
                                                     Proj proj = {}) const {
            auto const distance = container::distance(container);
            return equal_range_with_size(container::begin(container), needle, util::ref(comp), util::ref(proj),
                                         distance);
        }

    private:
        template<typename It, typename T, typename Proj, typename Comp,
                 typename SSizeType = meta::IteratorSSizeType<It>>
        constexpr static View<It> equal_range_with_size(It first, T const& needle, Comp comp, Proj proj,
                                                        meta::TypeIdentity<SSizeType> n) {
            return { LowerBoundFunction::lower_bound_with_size(first, needle, util::ref(comp), util::ref(proj), n),
                     UpperBoundFunction::upper_bound_with_size(first, needle, util::ref(comp), util::ref(proj), n) };
        }
    };
}

constexpr inline auto equal_range = detail::EqualRangeFunction {};
}