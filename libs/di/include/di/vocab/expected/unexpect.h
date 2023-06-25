#pragma once

namespace di::types {
struct Unexpect {
    constexpr explicit Unexpect() = default;
};

constexpr inline auto unexpect = Unexpect {};
}

namespace di {
using types::unexpect;
}
