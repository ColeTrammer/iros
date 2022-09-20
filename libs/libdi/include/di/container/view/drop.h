#pragma once

#include <di/container/view/drop_view.h>
#include <di/function/curry_back.h>
#include <di/function/tag_invoke.h>

namespace di::container::view {
namespace detail {
    struct DropFunction {
        template<concepts::ViewableContainer Con, concepts::ConvertibleTo<meta::ContainerSSizeType<Con>> Diff>
        constexpr concepts::View auto operator()(Con&& container, Diff&& difference) const {
            if constexpr (concepts::TagInvocable<DropFunction, Con, Diff>) {
                return function::tag_invoke(*this, util::forward<Con>(container), util::forward<Diff>(difference));
            } else {
                return DropView { util::forward<Con>(container), static_cast<meta::ContainerSSizeType<Con>>(difference) };
            }
        }
    };
}

constexpr inline auto drop = function::curry_back(detail::DropFunction {});
}