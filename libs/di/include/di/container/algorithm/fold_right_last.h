#pragma once

#include "di/container/algorithm/fold_right.h"
#include "di/container/concepts/prelude.h"
#include "di/container/iterator/prelude.h"
#include "di/container/meta/prelude.h"
#include "di/vocab/optional/prelude.h"

namespace di::container {
namespace detail {
    struct FoldRightLastFunction {
        template<concepts::BidirectionalIterator It, concepts::SentinelFor<It> Sent,
                 concepts::IndirectlyBinaryRightFoldable<meta::IteratorValue<It>, It> Op>
        requires(concepts::ConstructibleFrom<meta::IteratorValue<It>, meta::IteratorReference<It>>)
        constexpr auto operator()(It first, Sent last, Op op) const {
            using R = meta::Decay<meta::InvokeResult<Op&, meta::IteratorReference<It>, meta::IteratorValue<It>>>;
            using Res = Optional<R>;
            if (first == last) {
                return Res();
            }
            auto it_last = container::prev(container::next(first, last));
            return Res(in_place, container::fold_right(util::move(first), it_last, meta::IteratorValue<It>(*it_last),
                                                       util::ref(op)));
        }

        template<concepts::BidirectionalContainer Con,
                 concepts::IndirectlyBinaryRightFoldable<meta::ContainerValue<Con>, meta::ContainerIterator<Con>> Op>
        requires(concepts::ConstructibleFrom<meta::ContainerValue<Con>, meta::ContainerReference<Con>>)
        constexpr auto operator()(Con&& container, Op op) const {
            return (*this)(container::begin(container), container::end(container), util::ref(op));
        }
    };
}

constexpr inline auto fold_right_last = detail::FoldRightLastFunction {};
}

namespace di {
using container::fold_right_last;
}
