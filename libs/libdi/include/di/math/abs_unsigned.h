#pragma once

#include <di/concepts/integer.h>
#include <di/concepts/signed_integer.h>
#include <di/function/pipeline.h>
#include <di/math/numeric_limits.h>
#include <di/meta/make_unsigned.h>

namespace di::math {
namespace detail {
    struct AbsUnsignedFunction : function::pipeline::EnablePipeline {
        template<concepts::Integral T>
        constexpr auto operator()(T value) const {
            using R = meta::MakeUnsigned<T>;
            if constexpr (concepts::SignedIntegral<T>) {
                if (value == NumericLimits<T>::min) {
                    return R(value);
                }
                if (value < 0) {
                    return R(-value);
                }
            }
            return R(value);
        }
    };
}

constexpr inline auto abs_unsigned = detail::AbsUnsignedFunction {};
}