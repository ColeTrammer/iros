#pragma once

#include <di/container/algorithm/in_out_result.h>
#include <di/container/concepts/prelude.h>
#include <di/container/iterator/prelude.h>
#include <di/container/meta/prelude.h>
#include <di/meta/trivial.h>
#include <di/util/addressof.h>
#include <di/util/construct_at.h>
#include <di/util/destroy_at.h>
#include <di/util/relocate.h>

namespace di::container {
template<typename In, typename Out>
using UninitializedRelocateBackwardsResult = container::InOutResult<In, Out>;

namespace detail {
    struct UninitializedRelocateBackwardsFunction {
        template<concepts::BidirectionalIterator In, concepts::SentinelFor<In> Sent,
                 concepts::UninitBidirectionalIterator Out, concepts::UninitSentinelFor<Out> OutSent>
        requires(concepts::ConstructibleFrom<meta::IteratorValue<Out>, meta::IteratorValue<In>> &&
                 concepts::Destructible<meta::IteratorValue<In>>)
        constexpr UninitializedRelocateBackwardsResult<In, Out> operator()(In input, Sent in_sent, Out output,
                                                                           OutSent out_sent) const {
            auto in = container::next(input, in_sent);
            auto out = container::next(output, out_sent);

            // FIXME: add specical support for trivially relocatable types when not in constexpr context.
            while (in != input && out != output) {
                util::construct_at(util::addressof(*--out), util::relocate(*--in));
            }
            return { util::move(in), util::move(out) };
        }

        template<concepts::BidirectionalContainer Con, concepts::UninitBidirectionalContainer Out>
        requires(concepts::ConstructibleFrom<meta::ContainerValue<Out>, meta::ContainerValue<Con>> &&
                 concepts::Destructible<meta::ContainerValue<Con>>)
        constexpr UninitializedRelocateBackwardsResult<meta::BorrowedIterator<Con>, meta::BorrowedIterator<Out>>
        operator()(Con&& in, Out&& out) const {
            return (*this)(container::begin(in), container::end(in), container::begin(out), container::end(out));
        }
    };
}

constexpr inline auto uninitialized_relocate_backwards = detail::UninitializedRelocateBackwardsFunction {};
}
