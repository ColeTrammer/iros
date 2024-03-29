#pragma once

namespace di::util {
constexpr bool is_constant_evaluated() noexcept {
    if consteval {
        return true;
    } else {
        return false;
    }
}
}

namespace di {
using util::is_constant_evaluated;
}
