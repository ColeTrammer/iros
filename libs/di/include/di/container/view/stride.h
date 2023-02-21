#pragma once

#include <di/container/view/stride_view.h>
#include <di/function/curry_back.h>

namespace di::container::view {
namespace detail {
    struct StrideFunction;

    template<typename Con, typename SSizeType>
    concept CustomStride = concepts::TagInvocable<StrideFunction, Con, SSizeType>;

    template<typename Con, typename SSizeType>
    concept ViewStride = requires(Con&& container, SSizeType&& predicate) {
                             StrideView { util::forward<Con>(container), util::forward<SSizeType>(predicate) };
                         };

    struct StrideFunction {
        template<concepts::ViewableContainer Con, typename SSizeType>
        requires(CustomStride<Con, SSizeType> || ViewStride<Con, SSizeType>)
        constexpr concepts::View auto operator()(Con&& container, SSizeType&& predicate) const {
            if constexpr (CustomStride<Con, SSizeType>) {
                return function::tag_invoke(*this, util::forward<Con>(container), util::forward<SSizeType>(predicate));
            } else {
                return StrideView { util::forward<Con>(container), util::forward<SSizeType>(predicate) };
            }
        }
    };
}

constexpr inline auto stride = function::curry_back(detail::StrideFunction {}, meta::size_constant<2>);
}
