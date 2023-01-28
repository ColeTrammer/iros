#pragma once

#include <di/container/algorithm/in_out_result.h>
#include <di/container/concepts/prelude.h>
#include <di/container/iterator/prelude.h>
#include <di/container/meta/prelude.h>

namespace di::container {
namespace detail {
    struct CopyBackwardFunction {
        template<concepts::BidirectionalIterator It, concepts::SentinelFor<It> Sent,
                 concepts::BidirectionalIterator Out>
        requires(concepts::IndirectlyCopyable<It, Out>)
        constexpr InOutResult<It, Out> operator()(It first, Sent last, Out output) const {
            // FIXME: use vectorized byte copy (::memcpy_backwards) when provided contiguous
            //        iterators to trivially copyable types.
            auto last_it = container::next(first, last);
            for (auto it = last_it; it != first;) {
                *--output = *--it;
            }
            return { util::move(last_it), util::move(output) };
        }

        template<concepts::BidirectionalContainer Con, concepts::BidirectionalIterator Out>
        requires(concepts::IndirectlyCopyable<meta::ContainerIterator<Con>, Out>)
        constexpr InOutResult<meta::BorrowedIterator<Con>, Out> operator()(Con&& container, Out output) const {
            return (*this)(container::begin(container), container::end(container), util::move(output));
        }
    };
}

constexpr inline auto copy_backward = detail::CopyBackwardFunction {};
}