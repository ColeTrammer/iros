#pragma once

namespace di::vocab::optional {
struct NullOpt {
    constexpr explicit NullOpt(int) {}
};

constexpr inline auto nullopt = NullOpt { 0 };
}
