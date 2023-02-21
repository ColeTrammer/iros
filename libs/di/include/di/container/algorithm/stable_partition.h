#pragma once

#include <di/container/algorithm/find_if.h>
#include <di/container/algorithm/find_if_not.h>
#include <di/container/algorithm/rotate.h>

namespace di::container {
namespace detail {
    struct StablePartitionFunction {
        template<concepts::InputIterator It, concepts::SentinelFor<It> Sent, typename Proj = function::Identity,
                 concepts::IndirectUnaryPredicate<meta::Projected<It, Proj>> Pred>
        requires(concepts::Permutable<It>)
        constexpr View<It> operator()(It first, Sent last, Pred pred, Proj proj = {}) const {
            // Find the first element that does not belong to the left of the partition point.
            auto pivot = container::find_if_not(util::move(first), last, util::ref(pred), util::ref(proj));
            if (pivot == last) {
                return { pivot, pivot };
            }

            // Swap any element which is out of place back into place.
            // To preserve the relative order of elements, identity an
            // entire chunk of misplaced elements using find_if and find_if_not.
            // Then, rotate this entire block into place.
            // TODO: try to allocate a temporary buffer to speed up the
            //       operation.
            auto it = pivot;
            while (it != last) {
                it = container::find_if(util::move(it), last, util::ref(pred), util::ref(proj));
                if (it == last) {
                    break;
                }
                auto next = container::find_if_not(it, last, util::ref(pred), util::ref(proj));
                pivot = container::rotate(util::move(pivot), util::move(it), next).begin();
                it = next;
            }
            return { util::move(pivot), util::move(it) };
        }

        template<concepts::InputContainer Con, typename Proj = function::Identity,
                 concepts::IndirectUnaryPredicate<meta::Projected<meta::ContainerIterator<Con>, Proj>> Pred>
        requires(concepts::Permutable<meta::ContainerIterator<Con>>)
        constexpr meta::BorrowedView<Con> operator()(Con&& container, Pred pred, Proj proj = {}) const {
            return (*this)(container::begin(container), container::end(container), util::ref(pred), util::ref(proj));
        }
    };
}

constexpr inline auto stable_partition = detail::StablePartitionFunction {};
}
