#pragma once

namespace di::container {
struct DefaultSentinel {};

constexpr inline auto default_sentinel = DefaultSentinel {};
}