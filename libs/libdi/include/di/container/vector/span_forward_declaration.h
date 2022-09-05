#pragma once

#include <di/types/size_t.h>

namespace di::container {
constexpr inline auto dynamic_extent = static_cast<types::size_t>(-1);

template<typename T, types::size_t extent = dynamic_extent>
class Span;
}
