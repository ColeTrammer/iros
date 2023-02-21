#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/meta/prelude.h>
#include <di/container/view/join_with_view.h>
#include <di/function/curry_back.h>
#include <di/function/tag_invoke.h>
#include <di/util/forward.h>

namespace di::container::view {
namespace detail {
    struct JoinWithFunction;

    template<typename Con, typename Pattern>
    concept CustomJoinWith = concepts::TagInvocable<JoinWithFunction, Con, Pattern>;

    template<typename Con, typename Pattern>
    concept ViewJoinWith = requires(Con&& container, Pattern&& pattern) {
                               JoinWithView { util::forward<Con>(container), util::forward<Pattern>(pattern) };
                           };

    struct JoinWithFunction {
        template<concepts::ViewableContainer Con, typename Pattern>
        requires(CustomJoinWith<Con, Pattern> || ViewJoinWith<Con, Pattern>)
        constexpr auto operator()(Con&& container, Pattern&& pattern) const {
            if constexpr (CustomJoinWith<Con, Pattern>) {
                return function::tag_invoke(*this, util::forward<Con>(container), util::forward<Pattern>(pattern));
            } else {
                return JoinWithView { util::forward<Con>(container), util::forward<Pattern>(pattern) };
            }
        }
    };
}

constexpr inline auto join_with = function::curry_back(detail::JoinWithFunction {}, meta::size_constant<2>);
}
