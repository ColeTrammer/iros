#pragma once

#include <di/execution/concepts/queryable.h>
#include <di/execution/concepts/scheduler.h>
#include <di/execution/query/forwarding_query.h>
#include <di/util/as_const.h>

namespace di::execution {
template<concepts::OneOf<SetValue, SetError, SetStopped> CPO>
struct GetCompletionScheduler : ForwardingQuery {
    template<concepts::Queryable T>
    requires(concepts::TagInvocable<GetCompletionScheduler, T const&>)
    constexpr concepts::Scheduler auto operator()(T const& env) const {
        return function::tag_invoke(*this, env);
    }
};

template<concepts::OneOf<SetValue, SetError, SetStopped> CPO>
constexpr inline auto get_completion_scheduler = GetCompletionScheduler<CPO> {};
}
