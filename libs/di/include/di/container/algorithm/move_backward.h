#pragma once

#include <di/container/algorithm/in_out_result.h>
#include <di/container/concepts/prelude.h>
#include <di/container/iterator/prelude.h>
#include <di/container/meta/prelude.h>

namespace di::container {
template<typename In, typename Out>
using MoveBackwardResult = InOutResult<In, Out>;

namespace detail {
    struct MoveBackwardFunction {
        template<concepts::BidirectionalIterator In, concepts::SentinelFor<In> Sent,
                 concepts::BidirectionalIterator Out>
        requires(concepts::IndirectlyMovable<In, Out>)
        constexpr MoveBackwardResult<In, Out> operator()(In first, Sent last, Out output) const {
            auto last_it = container::next(first, last);
            for (auto it = last_it; it != first;) {
                *--output = container::iterator_move(--it);
            }
            return { util::move(last_it), util::move(output) };
        }

        template<concepts::BidirectionalContainer Con, concepts::BidirectionalIterator Out>
        requires(concepts::IndirectlyMovable<meta::ContainerIterator<Con>, Out>)
        constexpr MoveBackwardResult<meta::BorrowedIterator<Con>, Out> operator()(Con&& container, Out output) const {
            return (*this)(begin(container), end(container), util::move(output));
        }
    };
}

constexpr inline auto move_backward = detail::MoveBackwardFunction {};
}
