#pragma once

#include <di/util/tag_invoke.h>

namespace di::vocab::optional {
constexpr inline struct SetNulloptFunction {
    template<typename T>
    requires(di::util::concepts::TagInvocable<SetNulloptFunction, T&>)
    constexpr void operator()(T& value) const {
        di::util::tag_invoke(*this, value);
    }
} set_nullopt {};
}
