#pragma once

#include <di/meta/language.h>
#include <di/meta/util.h>
#include <di/util/to_underlying.h>

namespace di::bit {
namespace detail {
    struct ByteswapFunction {
        template<concepts::IntegralOrEnum T>
        requires(concepts::UniqueObjectRepresentation<T>)
        constexpr T operator()(T value) const {
            if constexpr (concepts::Enum<T>) {
                return T((*this)(util::to_underlying(value)));
            } else {
                if constexpr (sizeof(T) == 1) {
                    return value;
                } else if constexpr (sizeof(T) == 2) {
                    return __builtin_bswap16(value);
                } else if constexpr (sizeof(T) == 4) {
                    return __builtin_bswap32(value);
                } else if constexpr (sizeof(T) == 8) {
                    return __builtin_bswap64(value);
                }
#ifdef DI_HAVE_128_BIT_INTEGERS
                else if constexpr (sizeof(T) == 16) {
                    return __builtin_bswap128(value);
                }
#endif
                else {
                    static_assert(concepts::AlwaysFalse<T>);
                }
            }
        }
    };
}

constexpr inline auto byteswap = detail::ByteswapFunction {};
}
