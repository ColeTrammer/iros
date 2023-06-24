#pragma once

#include <di/container/view/elements_view.h>
#include <di/function/pipeable.h>

namespace di::container::view {
namespace detail {
    template<size_t index>
    struct ElementsFunction : function::pipeline::EnablePipeline {
        template<concepts::ViewableContainer Con>
        requires(requires(Con&& container) {
            ElementsView<meta::AsView<Con>, index> { util::forward<Con>(container) };
        })
        constexpr auto operator()(Con&& container) const {
            return ElementsView<meta::AsView<Con>, index> { util::forward<Con>(container) };
        }
    };
}

template<size_t index>
constexpr inline auto elements = detail::ElementsFunction<index> {};
}

namespace di {
using view::elements;
}
