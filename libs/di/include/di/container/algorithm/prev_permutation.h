#pragma once

#include "di/container/algorithm/next_permutation.h"

namespace di::container {
namespace detail {
    struct PrevPermutationFunction {
        template<concepts::BidirectionalIterator It, concepts::SentinelFor<It> Sent, typename Comp = function::Compare,
                 typename Proj = function::Identity>
        requires(concepts::Sortable<It, Comp, Proj>)
        constexpr auto operator()(It first, Sent last, Comp comp = {}, Proj proj = {}) const -> InFoundResult<It> {
            return container::next_permutation(
                util::move(first), last,
                [&]<typename T, typename U>(
                    T&& a, U&& b) -> decltype(function::invoke(comp, util::forward<T>(a), util::forward<U>(b))) {
                    return 0 <=> function::invoke(comp, util::forward<T>(a), util::forward<U>(b));
                },
                util::ref(proj));
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

constexpr inline auto prev_permutation = detail::PrevPermutationFunction {};
}

namespace di {
using container::prev_permutation;
}
