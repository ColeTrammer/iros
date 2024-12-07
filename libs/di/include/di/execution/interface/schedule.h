#pragma once

#include "di/execution/concepts/sender.h"
#include "di/function/tag_invoke.h"

namespace di::execution {
namespace detail {
    struct ScheduleFunction {
        template<typename Scheduler>
        requires(concepts::TagInvocable<ScheduleFunction, Scheduler>)
        constexpr auto operator()(Scheduler&& scheduler) const -> concepts::Sender auto {
            return function::tag_invoke(*this, util::forward<Scheduler>(scheduler));
        }
    };
}

constexpr inline auto schedule = detail::ScheduleFunction {};
}
