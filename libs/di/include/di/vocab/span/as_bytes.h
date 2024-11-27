#pragma once

#include <di/vocab/span/span_dynamic_size.h>
#include <di/vocab/span/span_fixed_size.h>

namespace di::vocab {
namespace detail {
    struct AsBytesFunction {
        template<typename T, size_t N, size_t S = N == dynamic_extent ? dynamic_extent : sizeof(T) * N>
        auto operator()(Span<T, N> span) const -> Span<Byte const, S> {
            return { reinterpret_cast<Byte const*>(span.data()), span.size_bytes() };
        }
    };
}

constexpr inline auto as_bytes = detail::AsBytesFunction {};
}

namespace di {
using vocab::as_bytes;
}
