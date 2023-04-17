#pragma once

#include <di/util/prelude.h>

namespace iris::mm {
struct PhysicalAddressTag {
    using Type = uptr;

    constexpr static bool format_as_pointer = true;
};

using PhysicalAddress = di::StrongInt<PhysicalAddressTag>;
}
