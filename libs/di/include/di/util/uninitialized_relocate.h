#pragma once

#include <di/concepts/constructible_from.h>
#include <di/concepts/destructible.h>
#include <di/container/algorithm/in_out_result.h>
#include <di/container/concepts/input_container.h>
#include <di/container/concepts/input_iterator.h>
#include <di/container/concepts/sentinel_for.h>
#include <di/container/meta/iterator_rvalue.h>
#include <di/container/meta/iterator_value.h>
#include <di/util/addressof.h>
#include <di/util/construct_at.h>
#include <di/util/destroy_at.h>

namespace di::util {
template<typename In, typename Out>
using UninitializedRelocateResult = container::InOutResult<In, Out>;

namespace detail {
    struct UninitializedRelocateFunction {
        template<concepts::InputIterator In, concepts::SentinelFor<In> Sent, concepts::Iterator Out,
                 concepts::SentinelFor<Out> OutSent>
        requires(concepts::ConstructibleFrom<meta::IteratorValue<Out>, meta::IteratorRValue<In>> &&
                 concepts::Destructible<meta::IteratorValue<In>>)
        constexpr UninitializedRelocateResult<In, Out> operator()(In input, Sent in_sent, Out output,
                                                                  OutSent out_sent) const {
            // FIXME: add specical support for trivially relocatable types when not in constexpr context.
            for (; input != in_sent && output != out_sent; ++input, ++output) {
                construct_at(addressof(*output), container::iterator_move(input));
                destroy_at(addressof(*input));
            }
            return { util::move(input), util::move(output) };
        }
    };
}

constexpr inline auto uninitialized_relocate = detail::UninitializedRelocateFunction {};
}
