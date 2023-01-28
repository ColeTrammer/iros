#pragma once

#include <di/container/view/adjacent_transform_view.h>
#include <di/container/view/empty.h>
#include <di/container/view/zip_transform_view.h>
#include <di/function/curry_back.h>
#include <di/function/pipeline.h>
#include <di/function/tag_invoke.h>

namespace di::container::view {
namespace detail {
    template<size_t N>
    struct AdjacentTransformFunction {
        template<concepts::ViewableContainer Con, typename F>
        constexpr concepts::View auto operator()(Con&& container, F&& function) const {
            if constexpr (N == 0) {
                return zip_transform(util::forward<F>(function));
            } else {
                return AdjacentTransformView<meta::AsView<Con>, meta::Decay<F>, N>(util::forward<Con>(container),
                                                                                   util::forward<F>(function));
            }
        }
    };
}

template<size_t N>
constexpr inline auto adjacent_transform =
    function::curry_back(detail::AdjacentTransformFunction<N> {}, meta::size_constant<2>);
}