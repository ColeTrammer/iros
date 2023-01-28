#pragma once

#include <di/container/algorithm/in_out_result.h>
#include <di/container/concepts/prelude.h>
#include <di/container/iterator/prelude.h>
#include <di/container/meta/prelude.h>

namespace di::container {
namespace detail {
    struct CopyNFunction {
        template<concepts::InputIterator It, concepts::WeaklyIncrementable Out,
                 typename SSizeType = meta::IteratorSSizeType<It>>
        requires(concepts::IndirectlyCopyable<It, Out>)
        constexpr InOutResult<It, Out> operator()(It first, meta::TypeIdentity<SSizeType> n, Out output) const {
            // FIXME: use vectorized byte copy (::memcpy_forward) when provided contiguous
            //        iterators to trivially copyable types.
            for (SSizeType i = 0; i < n; ++i, ++first, ++output) {
                *output = *first;
            }
            return { util::move(first), util::move(output) };
        }
    };
}

constexpr inline auto copy_n = detail::CopyNFunction {};
}