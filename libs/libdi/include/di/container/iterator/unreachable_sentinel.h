#pragma once

namespace di::container {
struct UnreachableSentinel {
    template<typename T>
    constexpr friend bool operator==(UnreachableSentinel, T const&) {
        return false;
    }
};

constexpr inline auto unreachable_sentinel = UnreachableSentinel {};
}
