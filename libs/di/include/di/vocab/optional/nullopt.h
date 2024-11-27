#pragma once

#include <di/types/strong_ordering.h>

namespace di::vocab {
struct NullOpt {
    constexpr explicit NullOpt(int) {}

    constexpr friend auto operator==(NullOpt, NullOpt) -> bool { return true; }
    constexpr friend auto operator<=>(NullOpt, NullOpt) -> types::strong_ordering {
        return types::strong_ordering::equal;
    }
};

constexpr inline auto nullopt = NullOpt { 0 };
}

namespace di {
using vocab::nullopt;
}
