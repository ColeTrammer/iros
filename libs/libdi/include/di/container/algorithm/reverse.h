#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/iterator/next.h>
#include <di/container/meta/prelude.h>

namespace di::container {
namespace detail {
    struct ReverseFunction {
        template<concepts::BidirectionalIterator Iter, concepts::SentinelFor<Iter> Sent>
        requires(concepts::Permutable<Iter>)
        constexpr Iter operator()(Iter first, Sent sentinel_last) const {
            auto last = container::next(first, sentinel_last);
            for (auto it = first, jt = last; it != jt && it != --jt; ++it) {
                container::iterator_swap(it, jt);
            }
            return last;
        }

        template<concepts::BidirectionalContainer Con>
        requires(concepts::Permutable<meta::ContainerIterator<Con>>)
        constexpr meta::BorrowedIterator<Con> operator()(Con&& container) const {
            return (*this)(container::begin(container), container::end(container));
        }
    };
}

constexpr inline auto reverse = detail::ReverseFunction {};
}