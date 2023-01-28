#pragma once

namespace di::container {
struct Dangling {
    constexpr Dangling() = default;

    template<typename... Args>
    constexpr Dangling(Args&&...) {}
};
}
