#pragma once

#include <di/container/algorithm/in_value_result.h>
#include <di/container/concepts/prelude.h>
#include <di/container/meta/prelude.h>
#include <di/function/identity.h>
#include <di/function/invoke.h>
#include <di/meta/util.h>
#include <di/util/move.h>
#include <di/util/reference_wrapper.h>
#include <di/vocab/optional/prelude.h>

namespace di::container {
namespace detail {
    struct FoldLeftFirstWithIterFunction {
        template<concepts::InputIterator Iter, concepts::SentinelFor<Iter> Sent,
                 concepts::IndirectlyBinaryLeftFoldable<meta::IteratorValue<Iter>, Iter> Op>
        requires(concepts::ConstructibleFrom<meta::IteratorValue<Iter>, meta::IteratorReference<Iter>>)
        constexpr auto operator()(Iter first, Sent last, Op op) const {
            using R = meta::Decay<meta::InvokeResult<Op&, meta::IteratorReference<Iter>, meta::IteratorValue<Iter>>>;
            using Res = InValueResult<Iter, Optional<R>>;
            if (first == last) {
                return Res(util::move(first), nullopt);
            }

            auto result = Optional<R> { in_place, *first };
            for (++first; first != last; ++first) {
                result = function::invoke(op, *util::move(result), *first);
            }
            return Res(util::move(first), util::move(result));
        }

        template<concepts::InputContainer Con,
                 concepts::IndirectlyBinaryLeftFoldable<meta::ContainerValue<Con>, meta::ContainerIterator<Con>> Op>
        requires(concepts::ConstructibleFrom<meta::ContainerValue<Con>, meta::ContainerReference<Con>>)
        constexpr InValueResult<
            meta::BorrowedIterator<Con>,
            Optional<meta::Decay<meta::InvokeResult<Op&, meta::ContainerReference<Con>, meta::ContainerValue<Con>>>>>
        operator()(Con&& container, Op op) const {
            return (*this)(container::begin(container), container::end(container), util::ref(op));
        }
    };
}

constexpr inline auto fold_left_first_with_iter = detail::FoldLeftFirstWithIterFunction {};
}

namespace di {
using container::fold_left_first_with_iter;
}
