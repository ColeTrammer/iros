#pragma once

#include "di/function/tag_invoke.h"
#include "di/util/declval.h"
#include "di/util/forward.h"

namespace di::vocab {
struct GetValueFunction {
    template<typename T>
    constexpr static auto operator()(T&& value) -> di::meta::TagInvokeResult<GetValueFunction, T> {
        return di::function::tag_invoke(GetValueFunction {}, di::util::forward<T>(value));
    }
};

constexpr inline auto get_value = GetValueFunction {};

template<typename Storage>
using OptionalGetValue = decltype(get_value(util::declval<Storage>()));
}
