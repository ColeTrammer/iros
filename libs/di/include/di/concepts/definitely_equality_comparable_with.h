#pragma once

namespace di::concepts::detail {
template<typename T, typename U>
struct DefinitelyEqualityComparableWith {
    constexpr static bool value = false;
};
}
