#pragma once

#include <di/execution/algorithm/schedule_from.h>
#include <di/execution/concepts/prelude.h>
#include <di/execution/meta/prelude.h>
#include <di/execution/receiver/prelude.h>
#include <di/execution/types/prelude.h>
#include <di/function/curry_back.h>

namespace di::execution {
namespace transfer_ns {
    struct Function {
        template<concepts::Scheduler Sched, concepts::Sender Send>
        auto operator()(Send&& sender, Sched&& scheduler) const -> concepts::Sender auto {
            if constexpr (requires {
                              function::tag_invoke(*this, get_completion_scheduler<SetValue>(get_env(sender)),
                                                   util::forward<Send>(sender), util::forward<Sched>(scheduler));
                          }) {
                return function::tag_invoke(*this, get_completion_scheduler<SetValue>(get_env(sender)),
                                            util::forward<Send>(sender), util::forward<Sched>(scheduler));
            } else if constexpr (concepts::TagInvocable<Function, Send, Sched>) {
                return function::tag_invoke(*this, util::forward<Send>(sender), util::forward<Sched>(scheduler));
            } else {
                return schedule_from(util::forward<Sched>(scheduler), util::forward<Send>(sender));
            }
        }
    };
}

constexpr inline auto transfer = function::curry_back(transfer_ns::Function {}, meta::c_<2zu>);
}
