#pragma once

#include <di/vocab/span/span_dynamic_size.h>
#include <di/vocab/span/span_fixed_size.h>

namespace di::vocab {
namespace detail {
    struct AsWritableBytesFunction {
        template<typename T, size_t N, size_t S = N == dynamic_extent ? dynamic_extent : sizeof(T) * N>
        requires(!concepts::Const<T>)
        Span<Byte, S> operator()(Span<T, N> span) const {
            return { reinterpret_cast<Byte*>(span.data()), span.size_bytes() };
        }
    };
}

constexpr inline auto as_writable_bytes = detail::AsWritableBytesFunction {};
}
