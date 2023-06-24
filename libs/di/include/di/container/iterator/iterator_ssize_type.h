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
    constexpr types::ssize_t operator()(types::InPlaceType<T*>) const;

    template<typename T>
    requires(!concepts::TagInvocable<IteratorSSizeTypeFunction, types::InPlaceType<T>> && !concepts::Pointer<T> &&
             requires(T const& a, T const& b) {
                 { a - b } -> concepts::Integral;
             })
    constexpr meta::MakeSigned<decltype(util::declval<T>() - util::declval<T>())>
    operator()(types::InPlaceType<T>) const;
} iterator_ssize_type;
}
