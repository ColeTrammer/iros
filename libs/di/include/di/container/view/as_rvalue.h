#pragma once

#include <di/container/meta/container_reference.h>
#include <di/container/meta/container_rvalue.h>
#include <di/container/view/all.h>
#include <di/container/view/as_rvalue_view.h>
#include <di/function/pipeable.h>
#include <di/util/forward.h>

namespace di::container::view {
namespace detail {
    template<typename T>
    concept AllAsRValue = requires(T&& container) { all(util::forward<T>(container)); } &&
                          concepts::SameAs<meta::ContainerRValue<T>, meta::ContainerReference<T>>;

    template<typename T>
    concept AsRValueViewAsRValue = requires(T&& container) { AsRValueView { util::forward<T>(container) }; };

    struct AsRValueFunction : public function::pipeline::EnablePipeline {
        template<concepts::ViewableContainer Con>
        requires(AllAsRValue<Con> || AsRValueViewAsRValue<Con>)
        constexpr concepts::View auto operator()(Con&& container) const {
            if constexpr (AllAsRValue<Con>) {
                return all(util::forward<Con>(container));
            } else {
                return AsRValueView { util::forward<Con>(container) };
            }
        }
    };
}

constexpr inline auto as_rvalue = detail::AsRValueFunction {};
}

namespace di {
using view::as_rvalue;
}
