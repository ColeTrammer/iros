#pragma once

#include <di/function/pipeable.h>
#include <di/vocab/optional/optional_void.h>

namespace di::vocab {
namespace detail {
    struct LiftBoolFunction : function::pipeline::EnablePipeline {
        constexpr Optional<void> operator()(bool value) const { return Optional<void>(value); }
    };
}

constexpr inline auto lift_bool = detail::LiftBoolFunction {};
}
