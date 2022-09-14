#pragma once

#include <di/concepts/always_false.h>
#include <di/meta/assert_fail.h>

namespace di::util {
template<auto... values>
void compile_time_fail() {}
}
