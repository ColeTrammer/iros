#pragma once

#include <di/function/tag_invoke.h>
#include <di/types/in_place_type.h>

namespace di::function::monad {
struct EnableMonadFunction {
    template<typename T>
    constexpr auto operator()(types::InPlaceType<T>) const {
        if constexpr (concepts::TagInvocableTo<EnableMonadFunction, bool, types::InPlaceType<T>>) {
            return tag_invoke(*this, types::in_place_type<T>);
        } else {
            return false;
        }
    }
};

constexpr inline auto enable_monad = EnableMonadFunction {};
}
