#pragma once

namespace di::concepts {
// This concept is used with static_assert() to stop compilation
// if any provided type is not well-formed.
template<typename...>
concept AlwaysTrue = true;
}
