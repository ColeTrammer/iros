#pragma once

#include <di/meta/core.h>

namespace di::meta {
template<typename A, typename B>
constexpr inline bool matching_sig = false;

template<typename T, typename... A, typename U, typename... B>
constexpr inline bool matching_sig<T(A...), U(B...)> = concepts::SameAs<T(A&&...), U(B&&...)>;
}
