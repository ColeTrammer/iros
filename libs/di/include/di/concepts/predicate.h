#pragma once

#include <di/function/invoke.h>

namespace di::concepts {
template<typename F, typename... Args>
concept Predicate = InvocableTo<F, bool, Args...>;
}