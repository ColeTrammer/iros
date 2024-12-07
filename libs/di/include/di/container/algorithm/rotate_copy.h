#pragma once

#include "di/container/algorithm/copy.h"
#include "di/container/algorithm/in_out_result.h"
#include "di/container/concepts/prelude.h"
#include "di/container/iterator/prelude.h"
#include "di/container/meta/prelude.h"

namespace di::container {
namespace detail {
    struct RotateCopyFunction {
        template<concepts::ForwardIterator It, concepts::SentinelFor<It> Sent, concepts::WeaklyIncrementable Out>
        requires(concepts::IndirectlyCopyable<It, Out>)
        constexpr auto operator()(It first, It middle, Sent last, Out output) const -> InOutResult<It, Out> {
            auto copy_left = container::copy(middle, last, util::move(output));
            auto copy_right = container::copy(util::move(first), util::move(middle), util::move(copy_left.out));
            return { util::move(copy_left.in), util::move(copy_right.out) };
        }

        template<concepts::ForwardContainer Con, concepts::WeaklyIncrementable Out>
        requires(concepts::IndirectlyCopyable<meta::ContainerIterator<Con>, Out>)
        constexpr auto operator()(Con&& container, meta::ContainerIterator<Con> middle, Out output) const
            -> InOutResult<meta::BorrowedIterator<Con>, Out> {
            return (*this)(container::begin(container), util::move(middle), container::end(container),
                           util::move(output));
        }
    };
}

constexpr inline auto rotate_copy = detail::RotateCopyFunction {};
}

namespace di {
using container::rotate_copy;
}
