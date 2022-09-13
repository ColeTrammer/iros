#pragma once

#include <di/function/tag_invoke.h>
#include <di/types/in_place_type.h>

namespace di::container {
constexpr inline struct IteratorReferenceFunction {
    template<typename T>
    requires(concepts::TagInvocable<IteratorReferenceFunction, types::InPlaceType<T>>)
    constexpr auto operator()(types::InPlaceType<T> x) const -> decltype(function::tag_invoke(*this, x));

    template<typename T>
    constexpr T& operator()(types::InPlaceType<T*>) const;
} iterator_reference;
}