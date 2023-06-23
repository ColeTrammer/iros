#pragma once

#include <di/meta/core.h>

namespace di::meta {
template<bool is_const, typename T>
using MaybeConst = Conditional<is_const, T const, T>;
}
