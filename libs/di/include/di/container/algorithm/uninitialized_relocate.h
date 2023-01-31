#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/iterator/prelude.h>
#include <di/container/meta/prelude.h>
#include <di/util/addressof.h>
#include <di/util/construct_at.h>
#include <di/util/destroy_at.h>
#include <di/util/relocate.h>

namespace di::container {
template<typename In, typename Out>
using UninitializedRelocateResult = container::InOutResult<In, Out>;

namespace detail {
    struct UninitializedRelocateFunction {
        template<concepts::UninitInputIterator In, concepts::UninitSentinelFor<In> Sent, concepts::Iterator Out,
                 concepts::SentinelFor<Out> OutSent>
        requires(concepts::ConstructibleFrom<meta::IteratorValue<Out>, meta::IteratorValue<In>> &&
                 concepts::Destructible<meta::IteratorValue<In>>)
        constexpr UninitializedRelocateResult<In, Out> operator()(In input, Sent in_sent, Out output,
                                                                  OutSent out_sent) const {
            // FIXME: add specical support for trivially relocatable types when not in constexpr context.
            for (; input != in_sent && output != out_sent; ++input, ++output) {
                util::construct_at(util::addressof(*output), util::relocate(*input));
            }
            return { util::move(input), util::move(output) };
        }

        template<concepts::UninitInputContainer Con, concepts::Container Out>
        requires(concepts::ConstructibleFrom<meta::ContainerValue<Out>, meta::ContainerValue<Con>> &&
                 concepts::Destructible<meta::ContainerValue<Con>>)
        constexpr UninitializedRelocateResult<meta::BorrowedIterator<Con>, meta::BorrowedIterator<Out>>
        operator()(Con&& in, Out&& out) const {
            return (*this)(container::begin(in), container::end(in), container::begin(out), container::end(out));
        }
    };
}

constexpr inline auto uninitialized_relocate = detail::UninitializedRelocateFunction {};
}
