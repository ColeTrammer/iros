#pragma once

#include <di/container/view/enumerate_view.h>
#include <di/function/pipeable.h>
#include <di/function/tag_invoke.h>
#include <di/util/forward.h>

namespace di::container::view {
namespace detail {
    struct EnumerateFunction;

    template<typename Con>
    concept CustomEnumerate = concepts::TagInvocable<EnumerateFunction, Con>;

    template<typename Con>
    concept ViewEnumerate = requires(Con&& container) { EnumerateView { util::forward<Con>(container) }; };

    struct EnumerateFunction : function::pipeline::EnablePipeline {
        template<concepts::ViewableContainer Con>
        requires(CustomEnumerate<Con> || ViewEnumerate<Con>)
        constexpr auto operator()(Con&& container) const -> concepts::View auto {
            if constexpr (CustomEnumerate<Con>) {
                return function::tag_invoke(*this, util::forward<Con>(container));
            } else {
                return EnumerateView { util::forward<Con>(container) };
            }
        }
    };
}

constexpr inline auto enumerate = detail::EnumerateFunction {};
}

namespace di {
using view::enumerate;
}
