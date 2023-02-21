#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/iterator/next.h>
#include <di/container/meta/prelude.h>
#include <di/function/equal.h>
#include <di/function/identity.h>

namespace di::container {
namespace detail {
    struct AdjacentFindFunction {
        template<concepts::ForwardIterator Iter, concepts::SentinelFor<Iter> Sent, typename Proj = function::Identity,
                 concepts::IndirectBinaryPredicate<meta::Projected<Iter, Proj>> Pred = function::Equal>
        constexpr Iter operator()(Iter fast, Sent last, Pred pred = {}, Proj proj = {}) const {
            if (fast == last) {
                return fast;
            }
            auto slow = fast++;
            for (; fast != last; ++fast, ++slow) {
                if (function::invoke(pred, function::invoke(proj, *slow), function::invoke(proj, *fast))) {
                    return slow;
                }
            }
            return fast;
        }

        template<concepts::ForwardContainer Con, typename Proj = function::Identity,
                 concepts::IndirectBinaryPredicate<meta::Projected<meta::ContainerIterator<Con>, Proj>> Pred =
                     function::Equal>
        constexpr meta::BorrowedIterator<Con> operator()(Con&& container, Pred pred = {}, Proj proj = {}) const {
            return (*this)(container::begin(container), container::end(container), util::ref(pred), util::ref(proj));
        }
    };
}

constexpr inline auto adjacent_find = detail::AdjacentFindFunction {};
}
