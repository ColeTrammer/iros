#pragma once

#include <di/container/view/adjacent_view.h>
#include <di/container/view/empty.h>
#include <di/function/pipeline.h>
#include <di/function/tag_invoke.h>

namespace di::container::view {
namespace detail {
    template<size_t N>
    struct AdjacentFunction : function::pipeline::EnablePipeline {
        template<concepts::ViewableContainer Con>
        constexpr auto operator()(Con&& container) const -> concepts::View auto {
            if constexpr (N == 0) {
                return empty<Tuple<>>;
            } else {
                return AdjacentView<meta::AsView<Con>, N>(util::forward<Con>(container));
            }
        }
    };
}

template<size_t N>
constexpr inline auto adjacent = detail::AdjacentFunction<N> {};
}

namespace di {
using view::adjacent;
}
