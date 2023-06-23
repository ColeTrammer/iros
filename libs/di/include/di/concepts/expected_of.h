#pragma once

#include <di/concepts/expected.h>
#include <di/meta/core.h>
#include <di/meta/expected_value.h>

namespace di::concepts {
template<typename Exp, typename T>
concept ExpectedOf = Expected<Exp> && SameAs<meta::ExpectedValue<Exp>, T>;
}
