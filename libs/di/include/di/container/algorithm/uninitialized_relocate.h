#pragma once

#include <di/container/algorithm/in_out_result.h>
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
        template<concepts::InputIterator In, concepts::SentinelFor<In> Sent, concepts::UninitForwardIterator Out,
                 concepts::UninitSentinelFor<Out> OutSent>
        requires(concepts::ConstructibleFrom<meta::IteratorValue<Out>, meta::IteratorValue<In>> &&
                 concepts::Destructible<meta::IteratorValue<In>>)
        constexpr auto operator()(In input, Sent in_sent, Out output, OutSent out_sent) const
            -> UninitializedRelocateResult<In, Out> {
            // FIXME: add specical support for trivially relocatable types when not in constexpr context.
            for (; input != in_sent && output != out_sent; ++input, ++output) {
                util::construct_at(util::addressof(*output), util::relocate(*input));
            }
            return { util::move(input), util::move(output) };
        }

        template<concepts::InputContainer Con, concepts::UninitForwardContainer Out>
        requires(concepts::ConstructibleFrom<meta::ContainerValue<Out>, meta::ContainerValue<Con>> &&
                 concepts::Destructible<meta::ContainerValue<Con>>)
        constexpr auto operator()(Con&& in, Out&& out) const
            -> UninitializedRelocateResult<meta::BorrowedIterator<Con>, meta::BorrowedIterator<Out>> {
            return (*this)(container::begin(in), container::end(in), container::begin(out), container::end(out));
        }
    };
}

constexpr inline auto uninitialized_relocate = detail::UninitializedRelocateFunction {};
}

namespace di {
using container::uninitialized_relocate;
}
