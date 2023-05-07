#pragma once

namespace di::concepts::detail {
template<typename T, typename U>
struct DefinitelyThreeWayComparableWith {
    using Type = void;
};
}
