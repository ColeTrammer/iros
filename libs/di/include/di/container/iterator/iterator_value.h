#pragma once

#include <di/function/tag_invoke.h>
#include <di/types/prelude.h>

namespace di::container {
constexpr inline struct IteratorValueFunction {
    template<typename T>
    requires(concepts::TagInvocable<IteratorValueFunction, types::InPlaceType<T>>)
    constexpr auto operator()(types::InPlaceType<T> x) const -> decltype(function::tag_invoke(*this, x));

    template<typename T>
    constexpr auto operator()(types::InPlaceType<T*>) const -> InPlaceType<meta::RemoveCV<T>>;
} iterator_value;
}
