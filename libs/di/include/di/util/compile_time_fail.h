#pragma once

#include <di/meta/util.h>

namespace di::util {
template<auto... values>
void compile_time_fail() {}
}
