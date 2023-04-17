#pragma once

#include <di/concepts/integral.h>
#include <di/function/pipeline.h>
#include <di/meta/make_unsigned.h>

namespace di::math {
namespace detail {
    struct ToUnsignedFunction : function::pipeline::EnablePipeline {
        template<concepts::Integral T>
        constexpr auto operator()(T value) const {
            return static_cast<meta::MakeUnsigned<T>>(value);
        }
    };
}

constexpr inline auto to_unsigned = detail::ToUnsignedFunction {};
}
