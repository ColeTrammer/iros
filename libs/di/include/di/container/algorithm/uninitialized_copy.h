#pragma once

#include "di/container/algorithm/in_out_result.h"
#include "di/container/concepts/prelude.h"
#include "di/container/iterator/prelude.h"
#include "di/container/meta/prelude.h"
#include "di/util/addressof.h"
#include "di/util/construct_at.h"

namespace di::container {
namespace detail {
    struct UninitializedCopyFunction {
        template<concepts::InputIterator It, concepts::SentinelFor<It> Sent, concepts::UninitForwardIterator Out,
                 concepts::UninitSentinelFor<Out> OutSent>
        requires(concepts::ConstructibleFrom<meta::IteratorValue<Out>, meta::IteratorReference<It>>)
        constexpr auto operator()(It in, Sent in_last, Out out, OutSent out_last) const -> InOutResult<It, Out> {
            for (; in != in_last && out != out_last; ++in, ++out) {
                util::construct_at(util::addressof(*out), *in);
            }
            return { util::move(in), util::move(out) };
        }

        template<concepts::InputContainer Con, concepts::UninitForwardContainer Out>
        requires(concepts::ConstructibleFrom<meta::ContainerValue<Out>, meta::ContainerReference<Con>>)
        constexpr auto operator()(Con&& in, Out&& out) const
            -> InOutResult<meta::BorrowedIterator<Con>, meta::BorrowedIterator<Out>> {
            return (*this)(container::begin(in), container::end(in), container::begin(out), container::end(out));
        }
    };
}

constexpr inline auto uninitialized_copy = detail::UninitializedCopyFunction {};
}

namespace di {
using container::uninitialized_copy;
}
