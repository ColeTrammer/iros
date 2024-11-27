#pragma once

#include <di/container/algorithm/in_in_result.h>
#include <di/container/concepts/prelude.h>
#include <di/container/iterator/prelude.h>
#include <di/container/meta/prelude.h>

namespace di::container {
namespace detail {
    struct SwapRanges {
        template<concepts::InputIterator It1, concepts::SentinelFor<It1> Sent1, concepts::InputIterator It2,
                 concepts::SentinelFor<It2> Sent2>
        requires(concepts::IndirectlySwappable<It1, It2>)
        constexpr auto operator()(It1 first1, Sent1 last1, It2 first2, Sent2 last2) const -> InInResult<It1, It2> {
            for (; first1 != last1 && first2 != last2; ++first1, ++first2) {
                container::iterator_swap(first1, first2);
            }
            return { util::move(first1), util::move(first2) };
        }

        template<concepts::InputContainer Con1, concepts::InputContainer Con2>
        requires(concepts::IndirectlySwappable<meta::ContainerIterator<Con1>, meta::ContainerIterator<Con2>>)
        constexpr auto operator()(Con1&& container1, Con2&& container2) const
            -> InInResult<meta::BorrowedIterator<Con1>, meta::BorrowedIterator<Con2>> {
            return (*this)(container::begin(container1), container::end(container1), container::begin(container2),
                           container::end(container2));
        }
    };
}

constexpr inline auto swap_ranges = detail::SwapRanges {};
}

namespace di {
using container::swap_ranges;
}
