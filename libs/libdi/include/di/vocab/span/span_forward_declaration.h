#pragma once

#include <di/math/numeric_limits.h>
#include <di/types/size_t.h>

namespace di::vocab {
constexpr inline auto dynamic_extent = math::NumericLimits<types::size_t>::max;

template<typename T, types::size_t extent = dynamic_extent>
class Span;
}
