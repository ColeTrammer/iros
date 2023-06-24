#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/iterator/prelude.h>
#include <di/container/meta/prelude.h>

namespace di::container {
namespace detail {
    struct UpperBoundFunction {
        template<concepts::ForwardIterator It, concepts::SentinelFor<It> Sent, typename T,
                 typename Proj = function::Identity,
                 concepts::IndirectStrictWeakOrder<T const*, meta::Projected<It, Proj>> Comp = function::Compare>
        constexpr It operator()(It first, Sent last, T const& needle, Comp comp = {}, Proj proj = {}) const {
            auto const distance = container::distance(first, last);
            return upper_bound_with_size(util::move(first), needle, util::ref(comp), util::ref(proj), distance);
        }

        template<concepts::ForwardContainer Con, typename T, typename Proj = function::Identity,
                 concepts::IndirectStrictWeakOrder<T const*, meta::Projected<meta::ContainerIterator<Con>, Proj>> Comp =
                     function::Compare>
        constexpr meta::BorrowedIterator<Con> operator()(Con&& container, T const& needle, Comp comp = {},
                                                         Proj proj = {}) const {
            auto const distance = container::distance(container);
            return upper_bound_with_size(container::begin(container), needle, util::ref(comp), util::ref(proj),
                                         distance);
        }

    private:
        friend struct EqualRangeFunction;

        template<typename It, typename T, typename Proj, typename Comp,
                 typename SSizeType = meta::IteratorSSizeType<It>>
        constexpr static It upper_bound_with_size(It first, T const& needle, Comp comp, Proj proj,
                                                  meta::TypeIdentity<SSizeType> n) {
            while (n != 0) {
                SSizeType left_length = n >> 1;
                auto mid = container::next(first, left_length);
                if (function::invoke(comp, needle, function::invoke(proj, *mid)) >= 0) {
                    // needle is greater than every element in the range [first, mid].
                    n -= left_length + 1;
                    first = ++mid;
                } else {
                    // needle is less than or equal to every element in the range [mid, last).
                    n = left_length;
                }
            }
            return first;
        }
    };
}

constexpr inline auto upper_bound = detail::UpperBoundFunction {};
}

namespace di {
using container::upper_bound;
}
