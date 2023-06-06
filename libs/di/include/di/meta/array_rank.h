#pragma once

#include <di/types/prelude.h>

namespace di::meta {
template<typename T>
constexpr inline auto ArrayRank = 0zu;

template<typename T>
constexpr inline auto ArrayRank<T[]> = 1 + ArrayRank<T>;

template<typename T, usize N>
constexpr inline auto ArrayRank<T[N]> = 1 + ArrayRank<T>;
}
