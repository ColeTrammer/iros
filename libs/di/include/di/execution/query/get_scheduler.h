#pragma once

#include <di/execution/concepts/scheduler.h>
#include <di/execution/query/forwarding_query.h>
#include <di/function/tag_invoke.h>

namespace di::execution {
namespace detail {
    struct GetSchedulerFunction : ForwardingQuery {
        template<typename T>
        requires(concepts::TagInvocable<GetSchedulerFunction, T const&>)
        constexpr concepts::Scheduler auto operator()(T&& value) const {
            return function::tag_invoke(*this, util::as_const(value));
        }

        constexpr auto operator()() const;
    };
}

constexpr inline auto get_scheduler = detail::GetSchedulerFunction {};
}
