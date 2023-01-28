#pragma once

#include <di/function/tag_invoke.h>

namespace di::execution {
namespace detail {
    struct ForwardingEnvQuery {
        template<typename T>
        constexpr bool operator()(T tag) const {
            if constexpr (concepts::TagInvocableTo<ForwardingEnvQuery, bool, T>) {
                return function::tag_invoke(*this, tag);
            } else {
                return true;
            }
        }
    };
}

constexpr inline auto forwarding_env_query = detail::ForwardingEnvQuery {};
}