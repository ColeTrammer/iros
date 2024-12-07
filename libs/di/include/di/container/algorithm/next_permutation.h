#pragma once

#include "di/container/algorithm/in_found_result.h"
#include "di/container/algorithm/reverse.h"
#include "di/container/concepts/prelude.h"
#include "di/container/iterator/prelude.h"
#include "di/container/meta/prelude.h"
#include "di/function/compare.h"

namespace di::container {
namespace detail {
    struct NextPermutationFunction {
        template<concepts::BidirectionalIterator It, concepts::SentinelFor<It> Sent, typename Comp = function::Compare,
                 typename Proj = function::Identity>
        requires(concepts::Sortable<It, Comp, Proj>)
        constexpr auto operator()(It first, Sent last, Comp comp = {}, Proj proj = {}) const -> InFoundResult<It> {
            // If there are 0 or 1 elements, just return without doing anything.
            if (first == last) {
                return { util::move(first), false };
            }
            auto last_it = container::next(first, last);
            auto it = container::prev(last_it);
            if (first == it) {
                return { util::move(last_it), false };
            }

            // Get to the next permutation.
            for (;;) {
                auto current = it;
                if (function::invoke(comp, function::invoke(proj, *--it), function::invoke(proj, *current)) < 0) {
                    // The previous element (note it was decremented) is less than the current. Since this check is in a
                    // loop, it means that the range [it, last_it) is sorted in reverse order. Our goal in next
                    // permutation is to permuate the range such that the range is the lexicographic successor.

                    // Find the insertion point for *it, which will be the last element which will be
                    // the first element is is less than. Since this range is sorted, this is operation
                    // should binary search instead of a linear scan (assuming the range is large).
                    auto insertion_point = last_it;
                    while (function::invoke(comp, function::invoke(proj, *it),
                                            function::invoke(proj, *--insertion_point)) >= 0) {}

                    // Swap it with the insertion point, which now means that [current, last_it)
                    // is still sorted in reverse order.
                    container::iterator_swap(it, insertion_point);

                    // Since we've generated the next permutation, reverse the range [current, last_it]
                    // so it is not sorted properly.
                    container::reverse(current, last_it);
                    return { util::move(last_it), true };
                }

                if (it == first) {
                    // The range is sorted in reverse order, so there are no more permutations.
                    // Reverse the range to get back to a sorted range, and return false.
                    container::reverse(first, last_it);
                    return { util::move(last_it), false };
                }
            }
        }

        template<concepts::BidirectionalContainer Con, typename Comp = function::Compare,
                 typename Proj = function::Identity>
        requires(concepts::Sortable<meta::ContainerIterator<Con>, Comp, Proj>)
        constexpr auto operator()(Con&& container, Comp comp = {}, Proj proj = {}) const
            -> InFoundResult<meta::BorrowedIterator<Con>> {
            return (*this)(container::begin(container), container::end(container), util::ref(comp), util::ref(proj));
        }
    };
}

constexpr inline auto next_permutation = detail::NextPermutationFunction {};
}

namespace di {
using container::next_permutation;
}
