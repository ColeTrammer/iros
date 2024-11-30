#pragma once

#include <di/container/algorithm/fold_left_with_iter.h>
#include <di/function/curry_back.h>

namespace di::container {
namespace detail {
    struct FoldLeftFunction {
        template<concepts::InputIterator Iter, concepts::SentinelFor<Iter> Sent, typename T,
                 concepts::IndirectlyBinaryLeftFoldable<T, Iter> Op>
        constexpr auto operator()(Iter first, Sent last, T init, Op op) const {
            return util::move(
                fold_left_with_iter(util::move(first), util::move(last), util::move(init), util::ref(op)).value);
        }

        template<concepts::InputContainer Con, typename T,
                 concepts::IndirectlyBinaryLeftFoldable<T, meta::ContainerIterator<Con>> Op>
        constexpr auto operator()(Con&& container, T init, Op op) const {
            return (*this)(container::begin(container), container::end(container), util::move(init), util::ref(op));
        }
    };
}

constexpr inline auto fold_left = function::curry_back(detail::FoldLeftFunction {}, meta::c_<3ZU>);
}

namespace di {
using container::fold_left;
}
