#pragma once

#include "di/types/strong_ordering.h"

namespace di::types {
struct Void {
    constexpr friend auto operator==(Void, Void) -> bool { return true; }
    constexpr friend auto operator<=>(Void, Void) -> types::strong_ordering { return types::strong_ordering::equal; }
};
}

namespace di {
using types::Void;
}
