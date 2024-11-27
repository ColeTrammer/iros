#pragma once

#include <di/container/view/join_view.h>
#include <di/function/pipeline.h>

namespace di::container::view {
namespace detail {
    struct JoinFunction;

    template<typename Con>
    concept CustomJoin = concepts::TagInvocable<JoinFunction, Con>;

    template<typename Con>
    concept ViewJoin = requires(Con&& container) { JoinView { util::forward<Con>(container) }; };

    struct JoinFunction : function::pipeline::EnablePipeline {
        template<concepts::ViewableContainer Con>
        requires(CustomJoin<Con> || ViewJoin<Con>)
        constexpr auto operator()(Con&& container) const -> concepts::View auto {
            if constexpr (CustomJoin<Con>) {
                return function::tag_invoke(*this, util::forward<Con>(container));
            } else {
                return JoinView { util::forward<Con>(container) };
            }
        }
    };

}

constexpr inline auto join = detail::JoinFunction {};
}

namespace di {
using view::join;
}
