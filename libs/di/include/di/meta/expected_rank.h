#pragma once

#include <di/concepts/expected.h>
#include <di/meta/expected_value.h>

namespace di::meta {
template<typename T>
constexpr inline auto ExpectedRank = 0zu;

template<concepts::Expected T>
constexpr inline auto ExpectedRank<T> = 1 + ExpectedRank<ExpectedValue<T>>;
}
