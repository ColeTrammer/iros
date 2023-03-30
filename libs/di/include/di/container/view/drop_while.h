#pragma once

#include <di/container/view/drop_while_view.h>
#include <di/function/curry_back.h>
#include <di/function/tag_invoke.h>
#include <di/util/forward.h>

namespace di::container::view {
namespace detail {
    struct DropWhileFunction;

    template<typename Con, typename Pred>
    concept CustomDropWhile = concepts::TagInvocable<DropWhileFunction, Con, Pred>;

    template<typename Con, typename Pred>
    concept ViewDropWhile = requires(Con&& container, Pred&& predicate) {
        DropWhileView { util::forward<Con>(container), util::forward<Pred>(predicate) };
    };

    struct DropWhileFunction {
        template<concepts::ViewableContainer Con, typename Pred>
        requires(CustomDropWhile<Con, Pred> || ViewDropWhile<Con, Pred>)
        constexpr concepts::View auto operator()(Con&& container, Pred&& predicate) const {
            if constexpr (CustomDropWhile<Con, Pred>) {
                return function::tag_invoke(*this, util::forward<Con>(container), util::forward<Pred>(predicate));
            } else {
                return DropWhileView { util::forward<Con>(container), util::forward<Pred>(predicate) };
            }
        }
    };
}

constexpr inline auto drop_while = function::curry_back(detail::DropWhileFunction {}, meta::size_constant<2>);
}
