#pragma once

#include <di/types/in_place_type.h>
#include <di/util/tag_invoke.h>

namespace di::container {
constexpr inline struct EnableViewFunction {
    template<typename T>
    constexpr auto operator()(types::InPlaceType<T> x) const {
        if constexpr (concepts::TagInvocableTo<EnableViewFunction, bool, decltype(x)>) {
            return util::tag_invoke(*this, x);
        } else {
            return false;
        }
    }
} enable_view;
}
