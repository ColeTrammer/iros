#pragma once

#include "di/container/view/slide_view.h"
#include "di/function/curry_back.h"

namespace di::container::view {
namespace detail {
    struct SlideFunction;

    template<typename Con, typename SSizeType>
    concept CustomSlide = concepts::TagInvocable<SlideFunction, Con, SSizeType>;

    template<typename Con, typename SSizeType>
    concept ViewSlide = requires(Con&& container, SSizeType&& predicate) {
        SlideView { util::forward<Con>(container), util::forward<SSizeType>(predicate) };
    };

    struct SlideFunction {
        template<concepts::ViewableContainer Con, typename SSizeType>
        requires(CustomSlide<Con, SSizeType> || ViewSlide<Con, SSizeType>)
        constexpr auto operator()(Con&& container, SSizeType&& predicate) const -> concepts::View auto {
            if constexpr (CustomSlide<Con, SSizeType>) {
                return function::tag_invoke(*this, util::forward<Con>(container), util::forward<SSizeType>(predicate));
            } else {
                return SlideView { util::forward<Con>(container), util::forward<SSizeType>(predicate) };
            }
        }
    };
}

constexpr inline auto slide = function::curry_back(detail::SlideFunction {}, meta::c_<2ZU>);
}

namespace di {
using view::slide;
}
