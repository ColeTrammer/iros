#pragma once

#include <di/execution/algorithm/repeat_effect_until.h>

namespace di::execution {
namespace repeat_effect_ns {
    struct Function : function::pipeline::EnablePipeline {
        template<concepts::SenderOf<NoEnv> Send>
        concepts::SenderOf<NoEnv> auto operator()(Send&& sender) const {
            if constexpr (concepts::TagInvocable<Function, Send>) {
                return function::tag_invoke(*this, util::forward<Send>(sender));
            } else {
                return execution::repeat_effect_until(util::forward<Send>(sender), [] {
                    return false;
                });
            }
        }
    };
}

constexpr inline auto repeat_effect = repeat_effect_ns::Function {};
}