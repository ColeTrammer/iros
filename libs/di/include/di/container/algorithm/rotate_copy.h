#pragma once

#include <di/container/algorithm/copy.h>
#include <di/container/algorithm/in_out_result.h>
#include <di/container/concepts/prelude.h>
#include <di/container/iterator/prelude.h>
#include <di/container/meta/prelude.h>

namespace di::container {
namespace detail {
    struct RotateCopyFunction {
        template<concepts::ForwardIterator It, concepts::SentinelFor<It> Sent, concepts::WeaklyIncrementable Out>
        requires(concepts::IndirectlyCopyable<It, Out>)
        constexpr InOutResult<It, Out> operator()(It first, It middle, Sent last, Out output) const {
            auto copy_left = container::copy(middle, last, util::move(output));
            auto copy_right = container::copy(util::move(first), util::move(middle), util::move(copy_left.out));
            return { util::move(copy_left.in), util::move(copy_right.out) };
        }

        template<concepts::ForwardContainer Con, concepts::WeaklyIncrementable Out>
        requires(concepts::IndirectlyCopyable<meta::ContainerIterator<Con>, Out>)
        constexpr InOutResult<meta::BorrowedIterator<Con>, Out>
        operator()(Con&& container, meta::ContainerIterator<Con> middle, Out output) const {
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
