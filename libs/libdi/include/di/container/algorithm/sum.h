#pragma once

#include <di/concepts/default_constructible.h>
#include <di/container/algorithm/fold_left.h>
#include <di/function/plus.h>

namespace di::container {
namespace detail {
    struct SumFunction {
        template<concepts::InputIterator Iter, concepts::SentinelFor<Iter> Sent,
                 concepts::DefaultConstructible T = meta::IteratorRValue<Iter>>
        requires(concepts::IndirectlyBinaryLeftFoldable<function::Plus, T, Iter>)
        constexpr auto operator()(Iter first, Sent last) const {
            return container::fold_left(util::move(first), last, T(), function::plus);
        }

        template<concepts::InputContainer Con, concepts::DefaultConstructible T = meta::ContainerValue<Con>>
        requires(concepts::IndirectlyBinaryLeftFoldable<function::Plus, T, meta::ContainerIterator<Con>>)
        constexpr auto operator()(Con&& container) const {
            return (*this)(container::begin(container), container::end(container));
        }
    };
}

constexpr inline auto sum = detail::SumFunction {};
}