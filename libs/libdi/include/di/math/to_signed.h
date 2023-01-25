#pragma once

#include <di/concepts/integral.h>
#include <di/function/pipeline.h>
#include <di/meta/make_signed.h>

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
