#pragma once

#include <di/execution/concepts/scheduler.h>
#include <di/function/tag_invoke.h>

namespace di::execution {
namespace detail {
    struct GetSchedulerFunction {
        template<typename T>
        requires(concepts::TagInvocable<GetSchedulerFunction, T const&>)
        constexpr concepts::Scheduler auto operator()(T const& value) const {
            return function::tag_invoke(*this, value);
        }

        constexpr auto operator()() const;
    };
}

constexpr inline auto get_scheduler = detail::GetSchedulerFunction {};
}
