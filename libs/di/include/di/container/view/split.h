#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/meta/prelude.h>
#include <di/container/view/split_view.h>
#include <di/function/curry_back.h>
#include <di/function/tag_invoke.h>
#include <di/util/forward.h>

namespace di::container::view {
namespace detail {
    struct SplitFunction;

    template<typename Con, typename Pattern>
    concept CustomSplit = concepts::TagInvocable<SplitFunction, Con, Pattern>;

    template<typename Con, typename Pattern>
    concept ViewSplit = requires(Con&& container, Pattern&& pattern) {
        SplitView { util::forward<Con>(container), util::forward<Pattern>(pattern) };
    };

    struct SplitFunction {
        template<concepts::ViewableContainer Con, typename Pattern>
        requires(CustomSplit<Con, Pattern> || ViewSplit<Con, Pattern>)
        constexpr concepts::View auto operator()(Con&& container, Pattern&& pattern) const {
            if constexpr (CustomSplit<Con, Pattern>) {
                return function::tag_invoke(*this, util::forward<Con>(container), util::forward<Pattern>(pattern));
            } else {
                return SplitView { util::forward<Con>(container), util::forward<Pattern>(pattern) };
            }
        }
    };
}

constexpr inline auto split = function::curry_back(detail::SplitFunction {}, meta::c_<2zu>);
}
