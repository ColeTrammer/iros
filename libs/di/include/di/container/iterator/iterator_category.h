#pragma once

#include <di/container/types/contiguous_iterator_tag.h>
#include <di/function/tag_invoke.h>
#include <di/types/in_place_type.h>

namespace di::container {
constexpr inline struct IteratorCategoryFunction {
    template<typename T>
    constexpr auto operator()(types::InPlaceType<T> x) const -> decltype(function::tag_invoke(*this, x));

    template<typename T>
    constexpr auto operator()(types::InPlaceType<T*>) const -> types::ContiguousIteratorTag;
} iterator_category;
}
