#pragma once

namespace di::vocab::optional {
struct NullOpt {
    constexpr explicit NullOpt(int) {}

    constexpr friend bool operator==(NullOpt, NullOpt) { return true; }
};

constexpr inline auto nullopt = NullOpt { 0 };
}
