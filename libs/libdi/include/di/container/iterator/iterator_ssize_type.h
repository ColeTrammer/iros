#pragma once

#include <di/types/in_place_type.h>
#include <di/types/ssize_t.h>
#include <di/util/tag_invoke.h>

namespace di::container {
constexpr inline struct IteratorSSizeTypeFunction {
    template<typename T>
    requires(concepts::TagInvocable<IteratorSSizeTypeFunction, types::InPlaceType<T>>)
    constexpr auto operator()(types::InPlaceType<T> x) const -> decltype(util::tag_invoke(*this, x));

    template<typename T>
    constexpr types::ssize_t operator()(types::InPlaceType<T*>) const;
} iterator_ssize_type;
}
