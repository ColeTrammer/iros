#pragma once

#include <di/function/tag_invoke.h>

namespace di::execution {
namespace detail {
    struct ForwardingSchedulerQuery {
        template<typename T>
        constexpr bool operator()(T tag) const {
            if constexpr (concepts::TagInvocableTo<ForwardingSchedulerQuery, bool, T>) {
                return function::tag_invoke(*this, tag);
            } else {
                return false;
            }
        }
    };
}

constexpr inline auto forwarding_scheduler_query = detail::ForwardingSchedulerQuery {};
}
