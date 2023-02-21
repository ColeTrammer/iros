#pragma once

#include <di/function/tag_invoke.h>

namespace di::execution {
namespace detail {
    struct ForwardingSenderQuery {
        template<typename T>
        constexpr bool operator()(T tag) const {
            if constexpr (concepts::TagInvocableTo<ForwardingSenderQuery, bool, T>) {
                return function::tag_invoke(*this, tag);
            } else {
                return false;
            }
        }
    };
}

constexpr inline auto forwarding_sender_query = detail::ForwardingSenderQuery {};
}
