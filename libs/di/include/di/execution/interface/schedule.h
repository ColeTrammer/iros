#pragma once

#include <di/execution/concepts/sender.h>
#include <di/function/tag_invoke.h>

namespace di::execution {
namespace detail {
    struct ScheduleFunction {
        template<typename Scheduler>
        requires(concepts::TagInvocable<ScheduleFunction, Scheduler>)
        constexpr concepts::Sender auto operator()(Scheduler&& scheduler) const {
            return function::tag_invoke(*this, util::forward<Scheduler>(scheduler));
        }
    };
}

constexpr inline auto schedule = detail::ScheduleFunction {};
}
