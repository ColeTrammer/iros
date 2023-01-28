#pragma once

#include <di/function/tag_invoke.h>
#include <di/util/forward.h>

namespace di::vocab {
constexpr inline struct SetValueFunction {
    template<typename T, typename... Args>
    constexpr auto operator()(T& value, Args&&... args) const
        -> di::meta::TagInvokeResult<SetValueFunction, T&, Args...> {
        return di::function::tag_invoke(*this, value, di::util::forward<Args>(args)...);
    }
} set_value {};
}
