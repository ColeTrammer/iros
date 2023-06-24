#pragma once

#include <di/container/algorithm/fold_left_first_with_iter.h>

namespace di::container {
namespace detail {
    struct FoldLeftFirstFunction {
        template<concepts::InputIterator Iter, concepts::SentinelFor<Iter> Sent,
                 concepts::IndirectlyBinaryLeftFoldable<meta::IteratorValue<Iter>, Iter> Op>
        requires(concepts::ConstructibleFrom<meta::IteratorValue<Iter>, meta::IteratorReference<Iter>>)
        constexpr auto operator()(Iter first, Sent last, Op op) const {
            return util::move(fold_left_first_with_iter(util::move(first), util::move(last), util::ref(op)).value);
        }

        template<concepts::InputContainer Con,
                 concepts::IndirectlyBinaryLeftFoldable<meta::ContainerValue<Con>, meta::ContainerIterator<Con>> Op>
        requires(concepts::ConstructibleFrom<meta::ContainerValue<Con>, meta::ContainerReference<Con>>)
        constexpr auto operator()(Con&& container, Op op) const {
            return (*this)(container::begin(container), container::end(container), util::ref(op));
        }
    };
}

constexpr inline auto fold_left_first = function::curry_back(detail::FoldLeftFirstFunction {}, meta::c_<2zu>);
}

namespace di {
using container::fold_left_first;
}
