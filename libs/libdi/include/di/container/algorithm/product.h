#pragma once

#include <di/container/algorithm/fold_left.h>
#include <di/function/multiplies.h>
#include <di/function/pipeline.h>

namespace di::container {
namespace detail {
    struct ProductFunction : function::pipeline::EnablePipeline {
        template<concepts::InputIterator Iter, concepts::SentinelFor<Iter> Sent, typename T = meta::IteratorValue<Iter>>
        requires(concepts::IndirectlyBinaryLeftFoldable<function::Multiplies, T, Iter> && requires { T(1); })
        constexpr auto operator()(Iter first, Sent last) const {
            return container::fold_left(util::move(first), util::move(last), T(1), function::multiplies);
        }

        template<concepts::InputContainer Con, typename T = meta::ContainerValue<Con>>
        requires(concepts::IndirectlyBinaryLeftFoldable<function::Multiplies, T, meta::ContainerIterator<Con>> &&
                 requires { T(1); })
        constexpr auto operator()(Con&& container) const {
            return (*this)(container::begin(container), container::end(container));
        }
    };
}

constexpr inline auto product = detail::ProductFunction {};
}