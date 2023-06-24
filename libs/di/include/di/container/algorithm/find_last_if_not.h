#pragma once

#include <di/container/algorithm/find_if_not.h>

namespace di::container {
namespace detail {
    struct FindLastIfNotFunction {
        template<concepts::InputIterator Iter, concepts::SentinelFor<Iter> Sent, typename Proj = function::Identity,
                 concepts::IndirectUnaryPredicate<meta::Projected<Iter, Proj>> Pred>
        constexpr View<Iter> operator()(Iter first, Sent last, Pred pred, Proj proj = {}) const {
            if constexpr (concepts::BidirectionalIterator<Iter> && concepts::SameAs<Iter, Sent>) {
                auto rlast = make_reverse_iterator(first);
                auto it = container::find_if_not(make_reverse_iterator(last), rlast, util::ref(pred), util::ref(proj));
                if (it == rlast) {
                    return { last, last };
                }
                return { container::prev(it.base()), last };
            } else {
                Iter result {};
                for (; first != last; ++first) {
                    if (!function::invoke(pred, function::invoke(proj, *first))) {
                        result = first;
                    }
                }
                return { result == Iter {} ? first : result, first };
            }
        }

        template<concepts::InputContainer Con, typename Proj = function::Identity,
                 concepts::IndirectUnaryPredicate<meta::Projected<meta::ContainerIterator<Con>, Proj>> Pred>
        constexpr meta::BorrowedIterator<Con> operator()(Con&& container, Pred pred, Proj proj = {}) const {
            return (*this)(container::begin(container), container::end(container), util::ref(pred), util::ref(proj));
        }
    };
}

constexpr inline auto find_last_if_not = detail::FindLastIfNotFunction {};
}

namespace di {
using container::find_last_if_not;
}
