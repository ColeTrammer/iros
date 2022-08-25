#pragma once

#include <di/util/declval.h>
#include <di/util/forward.h>
#include <di/util/tag_invoke.h>

namespace di::vocab::optional {
constexpr inline struct GetValueFunction {
    template<typename T>
    constexpr auto operator()(T&& value) const -> di::util::meta::TagInvokeResult<GetValueFunction, T> {
        return di::util::tag_invoke(*this, di::util::forward<T>(value));
    }
} get_value {};

template<typename Storage>
using OptionalGetValue = decltype(get_value(util::declval<Storage>()));
}
