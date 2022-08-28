#pragma once

#include <di/types/in_place_type.h>
#include <di/util/tag_invoke.h>

namespace di::container::iterator {
constexpr inline struct IteratorValueFunction {
    template<typename T>
    requires(concepts::TagInvocable<IteratorValueFunction, types::InPlaceType<T>>)
    constexpr auto operator()(types::InPlaceType<T> x) const -> decltype(util::tag_invoke(*this, x));

    template<typename T>
    constexpr T& operator()(types::InPlaceType<T*>) const;
} iterator_value;
}
