#pragma once

#include <di/types/prelude.h>

namespace di::meta {
template<typename T, types::size_t level = 0>
constexpr inline auto Extent = 0zu;

template<typename T, usize level>
constexpr inline auto Extent<T[], level> = Extent<T, level - 1>;

template<typename T, usize size>
constexpr inline auto Extent<T[size], 0> = size;

template<typename T, usize size, usize level>
constexpr inline auto Extent<T[size], level> = Extent<T, level - 1>;
}
