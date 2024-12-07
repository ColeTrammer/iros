#pragma once

#include "di/function/pipeline.h"
#include "di/meta/language.h"

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

namespace di {
using math::to_unsigned;
}
