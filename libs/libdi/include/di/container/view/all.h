#pragma once

#include <di/container/concepts/view.h>
#include <di/container/concepts/viewable_container.h>
#include <di/container/view/owning_view.h>
#include <di/container/view/ref_view.h>
#include <di/container/view/view_adapter.h>
#include <di/meta/decay.h>
#include <di/util/declval.h>
#include <di/util/forward.h>

namespace di::container::view {
namespace detail {
    template<typename Con>
    concept AllRefView = requires(Con&& container) { RefView { util::forward<Con>(container) }; };

    template<typename Con>
    concept AllOwningView = requires(Con&& container) { OwningView { util::forward<Con>(container) }; };

    struct AllFunction
        : view_adapter::ViewAdapter<AllFunction>
        , view_adapter::EnableViewClosure {
        template<concepts::ViewableContainer Con>
        requires(concepts::View<meta::Decay<Con>> || AllRefView<Con> || AllOwningView<Con>)
        constexpr auto operator()(Con&& container) const {
            if constexpr (concepts::View<meta::Decay<Con>>) {
                return util::forward<Con>(container);
            } else if constexpr (AllRefView<Con>) {
                return RefView { util::forward<Con>(container) };
            } else {
                return OwningView { util::forward<Con>(container) };
            }
        }

        using view_adapter::ViewAdapter<AllFunction>::operator();
    };
}

constexpr inline auto all = detail::AllFunction {};
}
