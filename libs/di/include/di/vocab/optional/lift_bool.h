#pragma once

#include <di/function/pipeable.h>
#include <di/vocab/optional/optional_void.h>

namespace di::vocab {
namespace detail {
    struct LiftBoolFunction : function::pipeline::EnablePipeline {
        constexpr auto operator()(bool value) const -> Optional<void> { return Optional<void>(value); }
    };
}

constexpr inline auto lift_bool = detail::LiftBoolFunction {};
}

namespace di {
using vocab::lift_bool;
}
