#pragma once

#include <di/container/meta/container_reference.h>
#include <di/container/meta/container_rvalue.h>
#include <di/container/view/all.h>
#include <di/container/view/as_const_view.h>
#include <di/function/pipeable.h>
#include <di/util/forward.h>

namespace di::container::view {
namespace detail {
    template<typename T>
    concept AllAsConst =
        requires(T&& container) { all(util::forward<T>(container)); } && concepts::ConstantContainer<meta::AsView<T>>;

    template<typename T>
    concept AsConstViewAsConst = requires(T&& container) { AsConstView { util::forward<T>(container) }; };

    struct AsConstFunction : public function::pipeline::EnablePipeline {
        template<concepts::ViewableContainer Con>
        requires(AllAsConst<Con> || AsConstViewAsConst<Con>)
        constexpr concepts::View auto operator()(Con&& container) const {
            if constexpr (AllAsConst<Con>) {
                return all(util::forward<Con>(container));
            } else {
                return AsConstView { util::forward<Con>(container) };
            }
        }
    };
}

constexpr inline auto as_const = detail::AsConstFunction {};
}
