#pragma once

#include <di/types/strong_ordering.h>

namespace di::vocab {
struct NullOpt {
    constexpr explicit NullOpt(int) {}

    constexpr friend bool operator==(NullOpt, NullOpt) { return true; }
    constexpr friend types::strong_ordering operator<=>(NullOpt, NullOpt) { return types::strong_ordering::equal; }
};

constexpr inline auto nullopt = NullOpt { 0 };
}
