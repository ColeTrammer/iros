#pragma once

namespace di::util {
constexpr auto is_constant_evaluated() noexcept -> bool {
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
