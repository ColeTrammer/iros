#pragma once

#include <di/util/tag_invoke.h>

namespace di::vocab::optional {
constexpr inline struct IsNulloptFunction {
    template<typename T>
    requires(concepts::TagInvocableTo<IsNulloptFunction, bool, T const&>)
    constexpr bool operator()(T const& value) const {
        return di::util::tag_invoke(*this, value);
    }
} is_nullopt {};
}
