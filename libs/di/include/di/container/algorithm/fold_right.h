#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/iterator/prelude.h>
#include <di/container/meta/prelude.h>

namespace di::container {
namespace detail {
    struct FoldRightFunction {
        template<concepts::BidirectionalIterator It, concepts::SentinelFor<It> Sent, typename T,
                 concepts::IndirectlyBinaryRightFoldable<T, It> Op>
        constexpr auto operator()(It first, Sent last, T init, Op op) const {
            using R = meta::Decay<meta::InvokeResult<Op&, meta::IteratorReference<It>, T>>;
            if (first == last) {
                return R(util::move(init));
            }
            auto it_last = container::next(first, last);
            R value = function::invoke(op, *--it_last, util::move(init));
            while (it_last != first) {
                value = function::invoke(op, *--it_last, util::move(value));
            }
            return value;
        }

        template<concepts::BidirectionalContainer Con, typename T,
                 concepts::IndirectlyBinaryRightFoldable<T, meta::ContainerIterator<Con>> Op>
        constexpr auto operator()(Con&& container, T init, Op op) const {
            return (*this)(container::begin(container), container::end(container), util::move(init), util::ref(op));
        }
    };
}

constexpr inline auto fold_right = detail::FoldRightFunction {};
}

namespace di {
using container::fold_right;
}
