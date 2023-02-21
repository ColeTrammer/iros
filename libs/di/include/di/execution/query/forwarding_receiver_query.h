#pragma once

#include <di/concepts/one_of.h>
#include <di/execution/receiver/set_error.h>
#include <di/execution/receiver/set_stopped.h>
#include <di/execution/receiver/set_value.h>
#include <di/function/tag_invoke.h>

namespace di::execution {
namespace detail {
    struct ForwardingReceiverQuery {
        template<typename T>
        constexpr bool operator()(T tag) const {
            if constexpr (concepts::TagInvocableTo<ForwardingReceiverQuery, bool, T>) {
                return function::tag_invoke(*this, tag);
            } else {
                return false;
            }
        }
    };
}

constexpr inline auto forwarding_receiver_query = detail::ForwardingReceiverQuery {};
}
