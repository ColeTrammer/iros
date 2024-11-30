#pragma once

#include <di/container/view/take_while_view.h>
#include <di/function/curry_back.h>
#include <di/function/tag_invoke.h>
#include <di/util/forward.h>

namespace di::container::view {
namespace detail {
    struct TakeWhileFunction;

    template<typename Con, typename Pred>
    concept CustomTakeWhile = concepts::TagInvocable<TakeWhileFunction, Con, Pred>;

    template<typename Con, typename Pred>
    concept ViewTakeWhile = requires(Con&& container, Pred&& predicate) {
        TakeWhileView { util::forward<Con>(container), util::forward<Pred>(predicate) };
    };

    struct TakeWhileFunction {
        template<concepts::ViewableContainer Con, typename Pred>
        requires(CustomTakeWhile<Con, Pred> || ViewTakeWhile<Con, Pred>)
        constexpr auto operator()(Con&& container, Pred&& predicate) const -> concepts::View auto {
            if constexpr (CustomTakeWhile<Con, Pred>) {
                return function::tag_invoke(*this, util::forward<Con>(container), util::forward<Pred>(predicate));
            } else {
                return TakeWhileView { util::forward<Con>(container), util::forward<Pred>(predicate) };
            }
        }
    };
}

constexpr inline auto take_while = function::curry_back(detail::TakeWhileFunction {}, meta::c_<2ZU>);
}

namespace di {
using view::take_while;
}
