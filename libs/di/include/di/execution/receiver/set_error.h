#pragma once

#include <di/function/tag_invoke.h>

namespace di::execution {
struct SetError {
    template<typename Receiver, typename Arg>
    requires(concepts::TagInvocable<SetError, Receiver, Arg>)
    constexpr void operator()(Receiver&& receiver, Arg&& arg) const {
        return function::tag_invoke(*this, util::forward<Receiver>(receiver), util::forward<Arg>(arg));
    }
};

constexpr inline auto set_error = SetError {};
}
