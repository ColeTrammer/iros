#pragma once

namespace di::util::concepts {
// This concept is used with static_assert() to cause the static assert
// to fail only when the template has been instantiated. This is useful
// for deleting function overloads.
template<typename...>
concept AlwaysFalse = false;
}
