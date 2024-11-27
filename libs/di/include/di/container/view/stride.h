#pragma once

#include <di/container/view/stride_view.h>
#include <di/function/curry_back.h>

namespace di::container::view {
namespace detail {
    struct StrideFunction;

    template<typename Con, typename SSizeType>
    concept CustomStride = concepts::TagInvocable<StrideFunction, Con, SSizeType>;

    template<typename Con, typename SSizeType>
    concept ViewStride = requires(Con&& container, SSizeType&& stride) {
        StrideView { util::forward<Con>(container), util::forward<SSizeType>(stride) };
    };

    struct StrideFunction {
        template<concepts::ViewableContainer Con, typename SSizeType = meta::ContainerSSizeType<Con>>
        requires(CustomStride<Con, SSizeType> || ViewStride<Con, SSizeType>)
        constexpr auto operator()(Con&& container, meta::TypeIdentity<SSizeType> stride) const -> concepts::View auto {
            if constexpr (CustomStride<Con, SSizeType>) {
                return function::tag_invoke(*this, util::forward<Con>(container), util::forward<SSizeType>(stride));
            } else {
                return StrideView { util::forward<Con>(container), util::forward<SSizeType>(stride) };
            }
        }
    };
}

constexpr inline auto stride = function::curry_back(detail::StrideFunction {}, meta::c_<2zu>);
}

namespace di {
using view::stride;
}
