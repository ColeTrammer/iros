#pragma once

#include <di/execution/concepts/scheduler.h>
#include <di/execution/query/forwarding_query.h>
#include <di/function/tag_invoke.h>
#include <di/util/as_const.h>

namespace di::execution {
namespace detail {
    struct GetDelegateeSchedulerFunction : ForwardingQuery {
        template<typename T>
        requires(concepts::TagInvocable<GetDelegateeSchedulerFunction, T const&>)
        constexpr auto operator()(T&& value) const -> concepts::Scheduler auto {
            return function::tag_invoke(*this, util::as_const(value));
        }

        constexpr auto operator()() const;
    };
}

constexpr inline auto get_delegatee_scheduler = detail::GetDelegateeSchedulerFunction {};
}
