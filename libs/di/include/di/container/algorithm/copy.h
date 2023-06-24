#pragma once

#include <di/container/algorithm/in_out_result.h>
#include <di/container/concepts/prelude.h>
#include <di/container/iterator/prelude.h>
#include <di/container/meta/prelude.h>

namespace di::container {
namespace detail {
    struct CopyFunction {
        template<concepts::InputIterator It, concepts::SentinelFor<It> Sent, concepts::WeaklyIncrementable Out>
        requires(concepts::IndirectlyCopyable<It, Out>)
        constexpr InOutResult<It, Out> operator()(It first, Sent last, Out output) const {
            // FIXME: use vectorized byte copy (::memcpy_forward) when provided contiguous
            //        iterators to trivially copyable types.
            for (; first != last; ++first, ++output) {
                *output = *first;
            }
            return { util::move(first), util::move(output) };
        }

        template<concepts::InputContainer Con, concepts::WeaklyIncrementable Out>
        requires(concepts::IndirectlyCopyable<meta::ContainerIterator<Con>, Out>)
        constexpr InOutResult<meta::BorrowedIterator<Con>, Out> operator()(Con&& container, Out output) const {
            return (*this)(container::begin(container), container::end(container), util::move(output));
        }
    };
}

constexpr inline auto copy = detail::CopyFunction {};
}

namespace di {
using container::copy;
}
