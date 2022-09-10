#pragma once

#include <di/types/prelude.h>
#include <di/vocab/span/span_fixed_size.h>

namespace di::vocab {
template<size_t count, typename T>
constexpr Span<T, count> fixed_span(T* value) {
    return Span<T, count>(value, count);
}
}
