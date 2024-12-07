#pragma once

#include "di/types/prelude.h"
#include "di/vocab/span/span_fixed_size.h"

namespace di::vocab {
template<size_t count, typename T>
constexpr auto fixed_span(T* value) -> Span<T, count> {
    return Span<T, count>(value, count);
}
}

namespace di {
using vocab::fixed_span;
}
