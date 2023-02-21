#pragma once

#include <di/function/tag_invoke.h>
#include <di/sync/concepts/stoppable_token.h>
#include <di/sync/stop_token/never_stop_token.h>

namespace di::execution {
namespace detail {
    struct GetStopTokenFunction {
        template<typename T>
        constexpr concepts::StoppableToken auto operator()(T const& value) const {
            if constexpr (concepts::TagInvocable<GetStopTokenFunction, T const&>) {
                return function::tag_invoke(*this, value);
            } else {
                return sync::NeverStopToken {};
            }
        }

        constexpr auto operator()() const;
    };
}

constexpr inline auto get_stop_token = detail::GetStopTokenFunction {};
}
