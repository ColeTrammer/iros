#pragma once

#include <di/container/view/transform_view.h>
#include <di/function/curry_back.h>
#include <di/function/tag_invoke.h>
#include <di/util/forward.h>

namespace di::container::view {
namespace detail {
    template<typename Con, typename F>
    concept RegularTransform = requires(Con&& container, F&& function) {
        TransformView { util::forward<Con>(container), util::forward<F>(function) };
    };

    struct TransformFunction {
        template<typename Con, typename F>
        requires(concepts::TagInvocable<TransformFunction, Con, F> || RegularTransform<Con, F>)
        constexpr auto operator()(Con&& container, F&& function) const {
            if constexpr (concepts::TagInvocable<TransformFunction, Con, F>) {
                return function::tag_invoke(*this, util::forward<Con>(container), util::forward<F>(function));
            } else {
                return TransformView { util::forward<Con>(container), util::forward<F>(function) };
            }
        }
    };
}

constexpr inline auto transform = function::curry_back(detail::TransformFunction {}, meta::size_constant<2>);
}
