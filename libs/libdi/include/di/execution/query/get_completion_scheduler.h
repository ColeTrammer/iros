#pragma once

#include <di/execution/concepts/scheduler.h>
#include <di/util/as_const.h>

namespace di::execution {
namespace detail {
    template<concepts::OneOf<SetValue, SetError, SetStopped> CPO>
    struct GetCompletionSchedulerFunction {
        template<concepts::Sender Sender>
        requires(concepts::TagInvocable<GetCompletionSchedulerFunction, Sender const&>)
        constexpr concepts::Scheduler auto operator()(Sender&& sender) const {
            return function::tag_invoke(*this, util::as_const(sender));
        }
    };
}

template<concepts::OneOf<SetValue, SetError, SetStopped> CPO>
constexpr inline auto get_completion_scheduler = detail::GetCompletionSchedulerFunction<CPO> {};
}