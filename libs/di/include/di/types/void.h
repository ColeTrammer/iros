#pragma once

#include <di/types/strong_ordering.h>

namespace di::types {
struct Void {
    constexpr friend bool operator==(Void, Void) { return true; }
    constexpr friend types::strong_ordering operator<=>(Void, Void) { return types::strong_ordering::equal; }
};
}

namespace di {
using types::Void;
}
