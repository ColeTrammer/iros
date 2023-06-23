#pragma once

#include <di/meta/algorithm.h>
#include <di/meta/core.h>
#include <di/meta/expected_value.h>

namespace di::meta {
template<typename T>
using UnwrapExpected = Invoke<Conditional<concepts::Expected<T>, meta::Quote<ExpectedValue>, meta::TypeConstant<T>>, T>;
}
