#pragma once

#include <di/concepts/constructible_from.h>
#include <di/concepts/destructible.h>
#include <di/container/algorithm/in_out_result.h>
#include <di/container/concepts/bidirectional_container.h>
#include <di/container/concepts/bidirectional_iterator.h>
#include <di/container/concepts/sentinel_for.h>
#include <di/container/iterator/distance.h>
#include <di/container/meta/iterator_rvalue.h>
#include <di/container/meta/iterator_value.h>
#include <di/util/addressof.h>
#include <di/util/construct_at.h>
#include <di/util/destroy_at.h>

namespace di::util {
template<typename In, typename Out>
using UninitializedRelocateBackwardsResult = container::InOutResult<In, Out>;

namespace detail {
    struct UninitializedRelocateBackwardsFunction {
        template<concepts::BidirectionalIterator In, concepts::SentinelFor<In> Sent,
                 concepts::BidirectionalIterator Out, concepts::SentinelFor<Out> OutSent>
        requires(concepts::ConstructibleFrom<meta::IteratorValue<Out>, meta::IteratorRValue<In>> &&
                 concepts::Destructible<meta::IteratorValue<In>>)
        constexpr UninitializedRelocateBackwardsResult<In, Out> operator()(In input, Sent in_sent, Out output,
                                                                           OutSent out_sent) const {
            auto in = input + container::distance(input, in_sent);
            auto out = output + container::distance(output, out_sent);

            // FIXME: add specical support for trivially relocatable types when not in constexpr context.
            while (in != input && out != output) {
                construct_at(addressof(*--out), container::iterator_move(--in));
                destroy_at(addressof(*in));
            }
            return { util::move(in), util::move(out) };
        }
    };
}

constexpr inline auto uninitialized_relocate_backwards = detail::UninitializedRelocateBackwardsFunction {};
}
