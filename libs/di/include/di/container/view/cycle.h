#pragma once

#include "di/container/view/cycle_view.h"
#include "di/function/pipeline.h"

namespace di::container::view {
namespace detail {
    struct CycleFunction;

    template<typename Con>
    concept CustomCycle = concepts::TagInvocable<CycleFunction, Con>;

    template<typename Con>
    concept ViewCycle = requires(Con&& container) { CycleView { util::forward<Con>(container) }; };

    struct CycleFunction : function::pipeline::EnablePipeline {
        template<concepts::ViewableContainer Con>
        requires(CustomCycle<Con> || ViewCycle<Con>)
        constexpr auto operator()(Con&& container) const -> concepts::View auto {
            if constexpr (CustomCycle<Con>) {
                return function::tag_invoke(*this, util::forward<Con>(container));
            } else {
                return CycleView { util::forward<Con>(container) };
            }
        }
    };

}

constexpr inline auto cycle = detail::CycleFunction {};
}

namespace di {
using view::cycle;
}
