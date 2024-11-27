#pragma once

#include <di/execution/query/forwarding_query.h>
#include <di/function/tag_invoke.h>
#include <di/sync/concepts/stoppable_token.h>
#include <di/sync/stop_token/never_stop_token.h>
#include <di/util/as_const.h>

namespace di::execution {
namespace detail {
    struct GetStopTokenFunction : ForwardingQuery {
        template<typename T>
        constexpr auto operator()(T&& value) const -> concepts::StoppableToken auto {
            if constexpr (concepts::TagInvocable<GetStopTokenFunction, T const&>) {
                return function::tag_invoke(*this, util::as_const(value));
            } else {
                return sync::NeverStopToken {};
            }
        }

        constexpr auto operator()() const;
    };
}

constexpr inline auto get_stop_token = detail::GetStopTokenFunction {};
}
