#pragma once

#include <di/concepts/conjunction.h>
#include <di/concepts/integral_or_enum.h>
#include <di/concepts/same_as.h>
#include <di/container/hash/default_hasher.h>
#include <di/container/hash/hash_write.h>
#include <di/function/tag_invoke.h>
#include <di/function/unpack.h>
#include <di/meta/make_index_sequence.h>
#include <di/meta/remove_cvref.h>
#include <di/types/prelude.h>
#include <di/vocab/tuple/tuple_element.h>
#include <di/vocab/tuple/tuple_like.h>
#include <di/vocab/tuple/tuple_size.h>

namespace di::container {
namespace detail {
    struct HashSameFunction {
        template<typename T, typename U>
        constexpr bool operator()(InPlaceType<T>, InPlaceType<U>) const {
            return concepts::SameAs<T, U>;
        }

        template<concepts::IntegralOrEnum T, concepts::IntegralOrEnum U>
        requires(!concepts::TagInvocable<HashWriteFunction, DefaultHasher&, T> &&
                 !concepts::TagInvocable<HashWriteFunction, DefaultHasher&, U>)
        constexpr bool operator()(InPlaceType<T>, InPlaceType<U>) const {
            return sizeof(T) == sizeof(U);
        }

        template<typename T, typename U>
        requires(concepts::TagInvocableTo<HashSameFunction, bool, InPlaceType<T>, InPlaceType<U>>)
        constexpr bool operator()(InPlaceType<T>, InPlaceType<U>) const {
            return function::tag_invoke(*this, in_place_type<T>, in_place_type<U>);
        }
    };
}

constexpr inline auto hash_same = detail::HashSameFunction {};
}

namespace di::concepts {
template<typename T, typename U>
concept HashSame = Hashable<T> && Hashable<U> &&
                   container::hash_same(in_place_type<meta::RemoveCVRef<T>>, in_place_type<meta::RemoveCVRef<U>>);
}

namespace di::container::detail {
template<HashableContainer T, HashableContainer U>
constexpr bool tag_invoke(types::Tag<hash_same>, InPlaceType<T>, InPlaceType<U>) {
    return concepts::HashSame<meta::ContainerReference<T>, meta::ContainerReference<U>>;
}

template<concepts::TupleLike T, concepts::TupleLike U>
requires((meta::TupleSize<T> == meta::TupleSize<U>) && !HashableContainer<T> && !HashableContainer<U>)
constexpr bool tag_invoke(types::Tag<hash_same>, InPlaceType<T>, InPlaceType<U>) {
    return function::unpack<meta::MakeIndexSequence<meta::TupleSize<T>>>(
        [&]<usize... indices>(meta::ListV<indices...>) {
            return concepts::Conjunction<
                concepts::HashSame<meta::TupleElement<T, indices>, meta::TupleElement<U, indices>>...>;
        });
}
}
