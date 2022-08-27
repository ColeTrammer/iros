#pragma once

#include <di/util/types/strong_ordering.h>

namespace di::vocab::optional {
struct NullOpt {
    constexpr explicit NullOpt(int) {}

    constexpr friend bool operator==(NullOpt, NullOpt) { return true; }
    constexpr friend util::types::strong_ordering operator<=>(NullOpt, NullOpt) { return util::types::strong_ordering::equal; }
};

constexpr inline auto nullopt = NullOpt { 0 };
}
