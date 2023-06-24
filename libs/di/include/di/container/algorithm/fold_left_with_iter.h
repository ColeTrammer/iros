#pragma once

#include <di/container/algorithm/in_value_result.h>
#include <di/container/concepts/prelude.h>
#include <di/container/meta/prelude.h>
#include <di/function/identity.h>
#include <di/function/invoke.h>
#include <di/meta/util.h>
#include <di/util/move.h>
#include <di/util/reference_wrapper.h>

namespace di::container {
namespace detail {
    struct FoldLeftWithIterFunction {
        template<concepts::InputIterator Iter, concepts::SentinelFor<Iter> Sent, typename T,
                 concepts::IndirectlyBinaryLeftFoldable<T, Iter> Op>
        constexpr auto operator()(Iter first, Sent last, T init, Op op) const {
            using R = meta::Decay<meta::InvokeResult<Op&, meta::IteratorReference<Iter>, T>>;
            using Res = InValueResult<Iter, R>;
            if (first == last) {
                return Res(util::move(first), R(util::move(init)));
            }

            R result = function::invoke(op, util::move(init), *first);
            for (++first; first != last; ++first) {
                result = function::invoke(op, util::move(result), *first);
            }
            return Res(util::move(first), util::move(result));
        }

        template<concepts::InputContainer Con, typename T,
                 concepts::IndirectlyBinaryLeftFoldable<T, meta::ContainerIterator<Con>> Op>
        constexpr InValueResult<meta::BorrowedIterator<Con>,
                                meta::Decay<meta::InvokeResult<Op&, meta::ContainerReference<Con>, T>>>
        operator()(Con&& container, T init, Op op) const {
            return (*this)(container::begin(container), container::end(container), util::move(init), util::ref(op));
        }
    };
}

constexpr inline auto fold_left_with_iter = detail::FoldLeftWithIterFunction {};
}
