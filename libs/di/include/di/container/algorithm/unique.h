#pragma once

#include <di/container/algorithm/adjacent_find.h>
#include <di/container/concepts/prelude.h>
#include <di/container/iterator/prelude.h>
#include <di/container/meta/prelude.h>

namespace di::container {
namespace detail {
    struct UniqueFunction {
        template<concepts::Permutable It, concepts::SentinelFor<It> Sent, typename Proj = function::Identity,
                 concepts::IndirectEquivalenceRelation<meta::Projected<It, Proj>> Comp = function::Equal>
        constexpr auto operator()(It first, Sent last, Comp comp = {}, Proj proj = {}) const -> View<It> {
            // Find the first element where duplicate elements occur.
            auto fast = container::adjacent_find(util::move(first), last, util::ref(comp), util::ref(proj));
            if (fast == last) {
                return { fast, fast };
            }

            // Since we know there are duplicates, advance the iterator to an
            // element which may or may not be a duplicate. Then, keep advancing the
            // fast iterator while it is a duplicate. Once it is not, output a new
            // value.
            auto slow = fast++;
            while (++fast != last) {
                if (!function::invoke(comp, function::invoke(proj, *slow), function::invoke(proj, *fast))) {
                    ++slow;
                    *slow = container::iterator_move(fast);
                }
            }
            return { container::next(slow), util::move(fast) };
        }

        template<concepts::ForwardContainer Con, typename Proj = function::Identity,
                 concepts::IndirectEquivalenceRelation<meta::Projected<meta::ContainerIterator<Con>, Proj>> Comp =
                     function::Equal>
        requires(concepts::Permutable<meta::ContainerIterator<Con>>)
        constexpr auto operator()(Con&& container, Comp comp = {}, Proj proj = {}) const -> meta::BorrowedView<Con> {
            return (*this)(container::begin(container), container::end(container), util::ref(comp), util::ref(proj));
        }
    };
}

constexpr inline auto unique = detail::UniqueFunction {};
}

namespace di {
using container::unique;
}
