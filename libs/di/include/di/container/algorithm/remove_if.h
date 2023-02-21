#pragma once

#include <di/container/algorithm/find_if.h>
#include <di/container/concepts/prelude.h>
#include <di/container/iterator/prelude.h>
#include <di/container/meta/prelude.h>

namespace di::container {
namespace detail {
    struct RemoveIfFunction {
        template<concepts::Permutable It, concepts::SentinelFor<It> Sent, typename Proj = function::Identity,
                 concepts::IndirectUnaryPredicate<meta::Projected<It, Proj>> Pred>
        constexpr View<It> operator()(It first, Sent last, Pred pred, Proj proj = {}) const {
            auto fast = container::find_if(util::move(first), last, util::ref(pred), util::ref(proj));
            if (fast == last) {
                return { fast, fast };
            }

            auto slow = fast++;
            for (; fast != last; ++fast) {
                if (!function::invoke(pred, function::invoke(proj, *fast))) {
                    *slow = container::iterator_move(fast);
                    ++slow;
                }
            }
            return { util::move(slow), util::move(fast) };
        }

        template<concepts::ForwardContainer Con, typename Proj = function::Identity,
                 concepts::IndirectUnaryPredicate<meta::Projected<meta::ContainerIterator<Con>, Proj>> Pred>
        requires(concepts::Permutable<meta::ContainerIterator<Con>>)
        constexpr meta::BorrowedView<Con> operator()(Con&& container, Pred pred, Proj proj = {}) const {
            return (*this)(container::begin(container), container::end(container), util::ref(pred), util::ref(proj));
        }
    };
}

constexpr inline auto remove_if = detail::RemoveIfFunction {};
}
