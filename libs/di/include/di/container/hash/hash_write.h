#pragma once

#include <di/concepts/integral_or_enum.h>
#include <di/concepts/tuple.h>
#include <di/container/concepts/forward_container.h>
#include <di/container/hash/default_hasher.h>
#include <di/container/hash/hasher.h>
#include <di/container/meta/container_reference.h>
#include <di/function/bind_front.h>
#include <di/function/tag_invoke.h>
#include <di/meta/constexpr.h>
#include <di/meta/index_sequence.h>
#include <di/meta/make_index_sequence.h>
#include <di/types/prelude.h>
#include <di/util/bit_cast.h>
#include <di/util/declval.h>
#include <di/util/reference_wrapper.h>
#include <di/vocab/array/array.h>
#include <di/vocab/tuple/tuple_for_each.h>
#include <di/vocab/tuple/tuple_like.h>
#include <di/vocab/tuple/tuple_value.h>

namespace di::container {
namespace detail {
    struct HashWriteFunction {
        constexpr void operator()(concepts::Hasher auto& hasher, vocab::Span<byte const> data) const {
            hasher.write(data);
        }

        template<concepts::IntegralOrEnum T>
        constexpr void operator()(concepts::Hasher auto& hasher, T value) const {
            auto bytes = util::bit_cast<vocab::Array<byte, sizeof(T)>>(value);
            hasher.write(bytes.span());
        }

        template<concepts::Hasher Hasher, typename T>
        requires(concepts::TagInvocable<HashWriteFunction, Hasher&, T const&>)
        constexpr void operator()(Hasher& hasher, T const& value) const {
            function::tag_invoke(*this, hasher, value);
        }
    };
}

constexpr inline auto hash_write = detail::HashWriteFunction {};
}

namespace di::concepts {
template<typename T>
concept Hashable = requires(container::DefaultHasher& hasher, T const& value) {
    { container::hash_write(hasher, value) } -> SameAs<void>;
};
}

namespace di::meta {
template<typename T>
struct Hashable : Constexpr<concepts::Hashable<T>> {};
}

namespace di::container::detail {
template<typename T>
concept HashableContainer = concepts::ForwardContainer<T> && concepts::Hashable<meta::ContainerReference<T>>;

constexpr void tag_invoke(types::Tag<hash_write>, concepts::Hasher auto& hasher, HashableContainer auto const& value) {
    for (auto const& element : value) {
        hash_write(hasher, element);
    }
}

namespace detail {
    template<typename Idx>
    struct HashableTuple {};

    template<usize... indices>
    struct HashableTuple<meta::IndexSequence<indices...>> {
        template<concepts::Tuple T>
        constexpr bool operator()(InPlaceType<T>) const {
            return (concepts::Hashable<meta::TupleValue<T const&, indices>> && ...);
        }
    };
}

template<concepts::TupleLike T, concepts::Hasher H>
requires(!HashableContainer<T> &&
         detail::HashableTuple<meta::MakeIndexSequence<meta::TupleSize<T>>>()(in_place_type<T>))
constexpr auto tag_invoke(types::Tag<hash_write>, H& hasher, T const& value) {
    return vocab::tuple_for_each(
        [&](concepts::Hashable auto const& x) {
            hash_write(hasher, x);
        },
        value);
}
}
