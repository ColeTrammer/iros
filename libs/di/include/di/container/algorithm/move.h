#pragma once

#include "di/container/algorithm/in_out_result.h"
#include "di/container/concepts/indirectly_movable.h"
#include "di/container/concepts/input_container.h"
#include "di/container/concepts/input_iterator.h"
#include "di/container/concepts/sentinel_for.h"
#include "di/container/concepts/weakly_incrementable.h"
#include "di/container/interface/begin.h"
#include "di/container/interface/end.h"
#include "di/container/iterator/iterator_move.h"
#include "di/container/meta/borrowed_iterator.h"
#include "di/container/meta/container_iterator.h"

namespace di::container {
template<typename In, typename Out>
using MoveResult = InOutResult<In, Out>;

namespace detail {
    struct MoveFunction {
        template<concepts::InputIterator In, concepts::SentinelFor<In> Sent, concepts::WeaklyIncrementable Out>
        requires(concepts::IndirectlyMovable<In, Out>)
        constexpr auto operator()(In first, Sent last, Out output) const -> MoveResult<In, Out> {
            for (; first != last; ++first, ++output) {
                *output = iterator_move(first);
            }
            return { util::move(first), util::move(output) };
        }

        template<concepts::InputContainer Con, concepts::WeaklyIncrementable Out>
        requires(concepts::IndirectlyMovable<meta::ContainerIterator<Con>, Out>)
        constexpr auto operator()(Con&& container, Out output) const -> MoveResult<meta::BorrowedIterator<Con>, Out> {
            return (*this)(begin(container), end(container), util::move(output));
        }
    };
}

constexpr inline auto move = detail::MoveFunction {};
}
