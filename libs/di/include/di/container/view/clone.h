#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/meta/prelude.h>
#include <di/container/view/all.h>
#include <di/container/view/transform.h>
#include <di/function/pipeable.h>
#include <di/meta/language.h>
#include <di/util/clone.h>
#include <di/util/forward.h>

namespace di::container::view {
namespace detail {
    struct CloneFunction : function::pipeline::EnablePipeline {
        template<concepts::ViewableContainer Con, typename Value = meta::ContainerReference<Con>>
        requires(!concepts::LValueReference<Value> || concepts::Clonable<Value>)
        constexpr concepts::View auto operator()(Con&& container) const {
            if constexpr (!concepts::LValueReference<Value>) {
                return all(util::forward<Con>(container));
            } else {
                return transform(util::forward<Con>(container), util::clone);
            }
        }
    };
}

constexpr inline auto clone = detail::CloneFunction {};
}
