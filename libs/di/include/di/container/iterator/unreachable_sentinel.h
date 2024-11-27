#pragma once

namespace di::container {
struct UnreachableSentinel {
    template<typename T>
    constexpr friend auto operator==(UnreachableSentinel, T const&) -> bool {
        return false;
    }
};

constexpr inline auto unreachable_sentinel = UnreachableSentinel {};
}
