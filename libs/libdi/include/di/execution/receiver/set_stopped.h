#pragma once

#include <di/function/tag_invoke.h>

namespace di::execution {
struct SetStopped {
    template<typename Receiver, typename Arg>
    requires(concepts::TagInvocable<SetStopped, Receiver>)
    constexpr void operator()(Receiver&& receiver) const {
        return function::tag_invoke(util::forward<Receiver>(receiver));
    }
};

constexpr inline auto set_stopped = SetStopped {};
}