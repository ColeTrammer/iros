#pragma once

#include "di/bit/endian/endian.h"
#include "di/function/tag_invoke.h"
#include "di/meta/core.h"
#include "di/meta/language.h"
#include "di/util/bit_cast.h"
#include "di/vocab/array/array.h"

namespace di::math {
namespace detail {
    struct AbsFunction {
        template<typename T>
        requires(concepts::TagInvocable<AbsFunction, T> || concepts::SignedIntegral<meta::RemoveCVRef<T>> ||
                 concepts::FloatingPoint<meta::RemoveCVRef<T>>)
        constexpr auto operator()(T&& value) const {
            if constexpr (concepts::TagInvocable<AbsFunction, T>) {
                return function::tag_invoke(*this, util::forward<T>(value));
            } else if constexpr (concepts::SignedIntegral<meta::RemoveCVRef<T>>) {
                return value < 0 ? -value : value;
            } else {
                auto as_bytes = di::bit_cast<di::Array<byte, sizeof(T)>>(value);
                auto& msb = [&] -> byte& {
                    if constexpr (Endian::Little == Endian::Native) {
                        return as_bytes[sizeof(T) - 1];
                    } else {
                        return as_bytes[0];
                    }
                }();

                msb &= ~(byte(1) << 7);

                return di::bit_cast<meta::RemoveCVRef<T>>(as_bytes);
            }
        }
    };
}

constexpr inline auto abs = detail::AbsFunction {};
}

namespace di {
using math::abs;
}
