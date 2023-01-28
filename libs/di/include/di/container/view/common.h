#pragma once

#include <di/container/meta/container_reference.h>
#include <di/container/meta/container_rvalue.h>
#include <di/container/view/all.h>
#include <di/container/view/common_view.h>
#include <di/function/pipeable.h>
#include <di/util/forward.h>

namespace di::container::view {
namespace detail {
    template<typename T>
    concept AllCommon = concepts::CommonContainer<T>;

    template<typename T>
    concept ViewCommon = requires(T&& container) { CommonView { util::forward<T>(container) }; };

    struct CommonFunction : public function::pipeline::EnablePipeline {
        template<concepts::ViewableContainer Con>
        requires(AllCommon<Con> || ViewCommon<Con>)
        constexpr concepts::View auto operator()(Con&& container) const {
            if constexpr (AllCommon<Con>) {
                return all(util::forward<Con>(container));
            } else {
                return CommonView { util::forward<Con>(container) };
            }
        }
    };
}

constexpr inline auto common = detail::CommonFunction {};
}
