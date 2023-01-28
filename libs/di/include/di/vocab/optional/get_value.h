#pragma once

#include <di/function/tag_invoke.h>
#include <di/util/declval.h>
#include <di/util/forward.h>

namespace di::vocab {
constexpr inline struct GetValueFunction {
    template<typename T>
    constexpr auto operator()(T&& value) const -> di::meta::TagInvokeResult<GetValueFunction, T> {
        return di::function::tag_invoke(*this, di::util::forward<T>(value));
    }
} get_value {};

template<typename Storage>
using OptionalGetValue = decltype(get_value(util::declval<Storage>()));
}
