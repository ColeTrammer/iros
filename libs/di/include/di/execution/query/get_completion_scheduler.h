#pragma once

#include <di/execution/concepts/scheduler.h>
#include <di/execution/query/forwarding_query.h>
#include <di/util/as_const.h>

namespace di::execution {
template<concepts::OneOf<SetValue, SetError, SetStopped> CPO>
struct GetCompletionScheduler : ForwardingQuery {
    template<concepts::Sender Sender>
    requires(concepts::TagInvocable<GetCompletionScheduler, Sender const&>)
    constexpr concepts::Scheduler auto operator()(Sender&& sender) const {
        return function::tag_invoke(*this, util::as_const(sender));
    }
};

template<concepts::OneOf<SetValue, SetError, SetStopped> CPO>
constexpr inline auto get_completion_scheduler = GetCompletionScheduler<CPO> {};
}
