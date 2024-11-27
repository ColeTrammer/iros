#pragma once

#include <di/function/tag_invoke.h>
#include <di/meta/language.h>
#include <di/types/in_place_type.h>
#include <di/types/ssize_t.h>
#include <di/util/declval.h>

namespace di::container {
constexpr inline struct IteratorSSizeTypeFunction {
    template<typename T>
    requires(concepts::TagInvocable<IteratorSSizeTypeFunction, types::InPlaceType<T>>)
    constexpr auto operator()(types::InPlaceType<T> x) const -> decltype(function::tag_invoke(*this, x));

    template<typename T>
    constexpr auto operator()(types::InPlaceType<T*>) const -> types::ssize_t;

    template<typename T>
    requires(!concepts::TagInvocable<IteratorSSizeTypeFunction, types::InPlaceType<T>> && !concepts::Pointer<T> &&
             requires(T const& a, T const& b) {
                 { a - b } -> concepts::Integral;
             })
    constexpr auto operator()(types::InPlaceType<T>) const
        -> meta::MakeSigned<decltype(util::declval<T>() - util::declval<T>())>;
} iterator_ssize_type;
}
