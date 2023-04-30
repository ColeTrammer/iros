#pragma once

#include <di/types/prelude.h>
#include <di/util/bitwise_enum.h>

namespace iris {
enum class OpenMode : u32 {
    None = 0,
    Create = (1 << 0),
    Mask = Create,
};

DI_DEFINE_ENUM_BITWISE_OPERATIONS(OpenMode)
}
