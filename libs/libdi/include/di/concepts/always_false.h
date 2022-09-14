#pragma once

namespace di::concepts {
// This concept is used with static_assert() to cause the static assert
// to fail only when the template has been instantiated. This is useful
// for failing compilation from within an if constexpxr block.
template<typename...>
concept AlwaysFalse = false;
}
