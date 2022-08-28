#pragma once

#include <di/container/types/contiguous_iterator_tag.h>
#include <di/types/in_place_type.h>
#include <di/util/tag_invoke.h>

namespace di::container::iterator {
constexpr inline struct IteratorCategoryFunction {
    template<typename T>
    constexpr auto operator()(types::InPlaceType<T> x) const -> decltype(util::tag_invoke(*this, x));

    template<typename T>
    constexpr types::ContiguousIteratorTag operator()(types::InPlaceType<T*>) const;
} iterator_category;
}
