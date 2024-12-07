#pragma once

#include "di/function/tag_invoke.h"

namespace di::execution {
struct SetValue {
    template<typename Receiver, typename... Args>
    requires(concepts::TagInvocable<SetValue, Receiver, Args...>)
    constexpr void operator()(Receiver&& receiver, Args&&... args) const {
        return function::tag_invoke(*this, util::forward<Receiver>(receiver), util::forward<Args>(args)...);
    }
};

constexpr inline auto set_value = SetValue {};
}

namespace di {
using execution::SetValue;
}
