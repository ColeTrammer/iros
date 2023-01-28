#pragma once

#include <di/function/tag_invoke.h>

namespace di::vocab {
constexpr inline struct IsNulloptFunction {
    template<typename T>
    requires(concepts::TagInvocableTo<IsNulloptFunction, bool, T const&>)
    constexpr bool operator()(T const& value) const {
        return di::function::tag_invoke(*this, value);
    }
} is_nullopt {};
}
