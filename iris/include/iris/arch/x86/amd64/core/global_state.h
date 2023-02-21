#pragma once

#include <di/prelude.h>

namespace iris::arch {
enum class CPUFeatures {
    Smep = 1 << 0,
    Smap = 1 << 1,
};

DI_DEFINE_ENUM_BITWISE_OPERATIONS(CPUFeatures)
}
