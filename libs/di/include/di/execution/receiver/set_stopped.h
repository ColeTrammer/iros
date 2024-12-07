#pragma once

#include "di/function/tag_invoke.h"

namespace di::execution {
struct SetStopped {
    template<typename Receiver>
    requires(concepts::TagInvocable<SetStopped, Receiver>)
    constexpr void operator()(Receiver&& receiver) const {
        return function::tag_invoke(*this, util::forward<Receiver>(receiver));
    }
};

constexpr inline auto set_stopped = SetStopped {};
}

namespace di {
using execution::SetStopped;
}
