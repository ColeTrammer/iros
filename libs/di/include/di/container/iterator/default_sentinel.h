#pragma once

namespace di::container {
struct DefaultSentinel {};

constexpr inline auto default_sentinel = DefaultSentinel {};
}

namespace di {
using container::default_sentinel;
using container::DefaultSentinel;
}
