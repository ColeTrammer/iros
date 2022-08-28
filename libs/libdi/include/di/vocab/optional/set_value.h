#pragma once

#include <di/util/forward.h>
#include <di/util/tag_invoke.h>

namespace di::vocab::optional {
constexpr inline struct SetValueFunction {
    template<typename T, typename... Args>
    constexpr auto operator()(T& value, Args&&... args) const -> di::meta::TagInvokeResult<SetValueFunction, T&, Args...> {
        return di::util::tag_invoke(*this, value, di::util::forward<Args>(args)...);
    }
} set_value {};
}
