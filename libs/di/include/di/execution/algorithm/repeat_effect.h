#pragma once

#include <di/execution/algorithm/repeat_effect_until.h>
#include <di/execution/receiver/set_value.h>

namespace di::execution {
namespace repeat_effect_ns {
    struct Function : function::pipeline::EnablePipeline {
        template<concepts::SenderOf<SetValue()> Send>
        auto operator()(Send&& sender) const -> concepts::SenderOf<SetValue()> auto {
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
