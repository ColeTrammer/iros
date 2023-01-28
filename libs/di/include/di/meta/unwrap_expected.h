#pragma once

#include <di/meta/conditional.h>
#include <di/meta/expected_value.h>
#include <di/meta/list/prelude.h>

namespace di::meta {
template<typename T>
using UnwrapExpected = Invoke<Conditional<concepts::Expected<T>, meta::Quote<ExpectedValue>, meta::Id<T>>, T>;
}