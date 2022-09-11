#pragma once

#include <di/concepts/constructible_from.h>
#include <di/concepts/destructible.h>
#include <di/container/concepts/input_container.h>
#include <di/container/concepts/input_iterator.h>
#include <di/container/concepts/sentinel_for.h>
#include <di/container/meta/iterator_rvalue.h>
#include <di/container/meta/iterator_value.h>
#include <di/util/address_of.h>
#include <di/util/construct_at.h>
#include <di/util/destroy_at.h>

namespace di::util {
namespace detail {
    struct UninitializedRelocateFunction {
        template<concepts::InputIterator Iter, concepts::SentinelFor<Iter> Sent, concepts::Iterator Out, concepts::SentinelFor<Out> OutSent>
        requires(concepts::ConstructibleFrom<meta::IteratorValue<Out>, meta::IteratorRValue<Iter>> &&
                 concepts::Destructible<meta::IteratorValue<Iter>>)
        constexpr void operator()(Iter input, Sent in_sent, Out output, OutSent out_sent) const {
            // FIXME: add specical support for trivially relocatable types when not in constexpr context.
            for (; input != in_sent && output != out_sent; ++input, ++output) {
                construct_at(address_of(*output), container::iterator_move(input));
                destroy_at(address_of(*input));
            }
        }
    };
}

constexpr inline auto uninitialized_relocate = detail::UninitializedRelocateFunction {};
}
