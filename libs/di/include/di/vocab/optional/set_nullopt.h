#pragma once

#include "di/function/tag_invoke.h"

namespace di::vocab {
constexpr inline struct SetNulloptFunction {
    template<typename T>
    requires(di::concepts::TagInvocable<SetNulloptFunction, T&>)
    constexpr void operator()(T& value) const {
        di::function::tag_invoke(*this, value);
    }
} set_nullopt {};
}
