#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/iterator/prelude.h>
#include <di/container/meta/prelude.h>

namespace di::container {
namespace detail {
    struct PartitionPointFunction {
        template<concepts::ForwardIterator It, concepts::SentinelFor<It> Sent, typename Proj = function::Identity,
                 concepts::IndirectUnaryPredicate<meta::Projected<It, Proj>> Pred>
        constexpr It operator()(It first, Sent last, Pred pred, Proj proj = {}) const {
            auto const n = container::distance(first, last);
            return partition_point_with_size(util::move(first), util::ref(pred), util::ref(proj), n);
        }

        template<concepts::ForwardContainer Con, typename Proj = function::Identity,
                 concepts::IndirectUnaryPredicate<meta::Projected<meta::ContainerIterator<Con>, Proj>> Pred>
        constexpr meta::BorrowedIterator<Con> operator()(Con&& container, Pred pred, Proj proj = {}) const {
            return partition_point_with_size(container::begin(container), util::ref(pred), util::ref(proj),
                                             container::distance(container));
        }

    private:
        template<typename It, typename Pred, typename Proj, typename SSizeType>
        constexpr static It partition_point_with_size(It first, Pred pred, Proj proj, SSizeType n) {
            // Perform binary search for the partition point, knowing the size of the container.
            // If the mid point cannot be the partition point, advance first past the just
            // checked point, otherwise decrease the length.
            while (n > 0) {
                auto const mid_length = static_cast<SSizeType>(n / 2);
                auto mid = container::next(first, mid_length);
                if (function::invoke(pred, function::invoke(proj, *mid))) {
                    // Mid is before the partition point.
                    // Skip the range denoted by [first, mid].
                    first = util::move(++mid);
                    n -= mid_length + 1;
                } else {
                    // Mid is the partition point or past it.
                    // Skip the trailing range denoted by [mid, last].
                    n = mid_length;
                }
            }
            return first;
        }
    };
}

constexpr inline auto partition_point = detail::PartitionPointFunction {};
}

namespace di {
using container::partition_point;
}
