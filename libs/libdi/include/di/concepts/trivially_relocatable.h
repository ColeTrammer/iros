#pragma once

#include <di/concepts/trivial.h>
#include <di/function/tag_invoke.h>
#include <di/types/prelude.h>

namespace di::concepts {
namespace detail {
    struct TriviallyRelocatableFunction {
        template<typename T>
        constexpr static bool operator()(InPlaceType<T>) const {
            if constexpr (TagInvocableTo<TriviallyRelocatableFunction, InPlaceType<T>>) {
                return function::tag_invoke(*this, in_place_type<T>);
            } else {
                return concepts::Trivial<T>;
            }
        }
    };
}

constexpr inline auto trivially_relocatable = detail::TriviallyRelocatableFunction {};

template<typename T>
concept TriviallyRelocatable = trivially_relocatable(in_place_type<T>);
}