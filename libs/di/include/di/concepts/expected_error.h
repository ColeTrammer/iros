#pragma once

#include <di/concepts/expected.h>
#include <di/meta/core.h>
#include <di/meta/expected_error.h>

namespace di::concepts {
template<typename Exp, typename T>
concept ExpectedError = Expected<Exp> && SameAs<meta::ExpectedError<Exp>, T>;
}
