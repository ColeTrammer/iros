#pragma once

#include "di/function/pipeline.h"
#include "di/meta/language.h"

namespace di::math {
namespace detail {
    struct ToSignedFunction : function::pipeline::EnablePipeline {
        template<concepts::Integral T>
        constexpr auto operator()(T value) const {
            return static_cast<meta::MakeSigned<T>>(value);
        }
    };
}

constexpr inline auto to_signed = detail::ToSignedFunction {};
}

namespace di {
using math::to_signed;
}
