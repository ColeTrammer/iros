#pragma once

#include <di/concepts/integral.h>
#include <di/concepts/pointer.h>
#include <di/meta/make_signed.h>
#include <di/types/in_place_type.h>
#include <di/types/ssize_t.h>
#include <di/util/declval.h>
#include <di/util/tag_invoke.h>

namespace di::container {
constexpr inline struct IteratorSSizeTypeFunction {
    template<typename T>
    requires(concepts::TagInvocable<IteratorSSizeTypeFunction, types::InPlaceType<T>>)
    constexpr auto operator()(types::InPlaceType<T> x) const -> decltype(util::tag_invoke(*this, x));

    template<typename T>
    constexpr types::ssize_t operator()(types::InPlaceType<T*>) const;

    template<typename T>
    requires(!concepts::TagInvocable<IteratorSSizeTypeFunction, types::InPlaceType<T>> && !concepts::Pointer<T> &&
             requires(T const& a, T const& b) {
                 { a - b } -> concepts::Integral;
             })
    constexpr meta::MakeSigned<decltype(util::declval<T>() - util::declval<T>())> operator()(types::InPlaceType<T>) const;
} iterator_ssize_type;
}
