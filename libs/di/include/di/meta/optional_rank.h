#pragma once

#include <di/concepts/optional.h>
#include <di/meta/optional_value.h>

namespace di::meta {
template<typename T>
constexpr inline auto OptionalRank = 0zu;

template<concepts::Optional T>
constexpr inline auto OptionalRank<T> = 1 + OptionalRank<OptionalValue<T>>;
}
