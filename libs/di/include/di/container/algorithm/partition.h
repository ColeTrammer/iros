#pragma once

#include <di/container/algorithm/find_if_not.h>

namespace di::container {
namespace detail {
    struct PartitionFunction {
        template<concepts::InputIterator It, concepts::SentinelFor<It> Sent, typename Proj = function::Identity,
                 concepts::IndirectUnaryPredicate<meta::Projected<It, Proj>> Pred>
        requires(concepts::Permutable<It>)
        constexpr View<It> operator()(It first, Sent last, Pred pred, Proj proj = {}) const {
            // Find the first element that does not belong to the left of the partition point.
            auto fast = container::find_if_not(util::move(first), last, util::ref(pred), util::ref(proj));
            if (fast == last) {
                return { fast, fast };
            }

            // Swap any element which is out of place back into place.
            auto slow = fast++;
            for (; fast != last; ++fast) {
                if (function::invoke(pred, function::invoke(proj, *fast))) {
                    container::iterator_swap(slow++, fast);
                }
            }
            return { util::move(slow), util::move(fast) };
        }

        template<concepts::InputContainer Con, typename Proj = function::Identity,
                 concepts::IndirectUnaryPredicate<meta::Projected<meta::ContainerIterator<Con>, Proj>> Pred>
        requires(concepts::Permutable<meta::ContainerIterator<Con>>)
        constexpr meta::BorrowedView<Con> operator()(Con&& container, Pred pred, Proj proj = {}) const {
            return (*this)(container::begin(container), container::end(container), util::ref(pred), util::ref(proj));
        }
    };
}

constexpr inline auto partition = detail::PartitionFunction {};
}

namespace di {
using container::partition;
}
