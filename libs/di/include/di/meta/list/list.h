#pragma once

#include <di/concepts/same_as.h>
#include <di/meta/list/concepts/type_list.h>
#include <di/meta/list/list_forward_declation.h>
#include <di/meta/size_constant.h>
#include <di/meta/type_constant.h>
#include <di/types/size_t.h>

namespace di::meta {
namespace detail {
    template<types::size_t index, typename... Types>
    struct AtHelper {};

    template<typename T, typename... Rest>
    struct AtHelper<0, T, Rest...> : TypeConstant<T> {};

    template<types::size_t index, typename T, typename... Rest>
    struct AtHelper<index, T, Rest...> : AtHelper<index - 1, Rest...> {};
}

namespace detail {
    template<typename Needle, typename... Types>
    struct LookupHelper : SizeConstant<0> {};

    template<typename Needle, typename T, typename... Rest>
    struct LookupHelper<Needle, T, Rest...>
        : SizeConstant<concepts::SameAs<T, Needle> ? 0 : 1 + LookupHelper<Needle, Rest...>::value> {};
}

namespace detail {
    template<typename... Types>
    struct BackHelper {};

    template<typename T>
    struct BackHelper<T> : TypeConstant<T> {};

    template<typename Head, typename... Tail>
    struct BackHelper<Head, Tail...> : BackHelper<Tail...> {};
}

template<typename Head, typename... Rest>
struct List<Head, Rest...> {
    using Front = Head;

    using Back = detail::BackHelper<Rest...>::Type;

    constexpr static size_t size = sizeof...(Rest) + 1;

    template<size_t index>
    using At = detail::AtHelper<index, Head, Rest...>::Type;

    template<typename T>
    constexpr static auto Lookup = detail::LookupHelper<T, Head, Rest...>::value;

    template<typename T>
    constexpr static bool UniqueType =
        (static_cast<size_t>(concepts::SameAs<T, Head>) + ... + static_cast<size_t>(concepts::SameAs<T, Rest>)) == 1zu;
};

template<typename T>
struct List<T> {
    using Front = T;
    using Back = T;

    constexpr static size_t size = 1;

    template<size_t index>
    using At = detail::AtHelper<index, T>::Type;

    template<typename U>
    constexpr static auto Lookup = detail::LookupHelper<U, T>::value;

    template<typename U>
    constexpr static bool UniqueType = concepts::SameAs<T, U>;
};

template<>
struct List<> {
    constexpr static size_t size = 0;

    template<typename U>
    constexpr static bool UniqueType = false;

    template<size_t index>
    requires(index != index)
    using At = void;

    template<size_t index>
    requires(index != index)
    using Front = void;

    template<size_t index>
    requires(index != index)
    using Back = void;

    template<typename U>
    constexpr static size_t Lookup = 0;
};

template<concepts::TypeList T>
using Front = T::Front;

template<concepts::TypeList T>
using Back = T::Back;

template<concepts::TypeList T>
constexpr inline size_t Size = T::size;

template<concepts::TypeList T, size_t index>
requires(index < Size<T>)
using At = typename T::At<index>;

template<typename T, concepts::TypeList List>
constexpr static inline auto Lookup = List::template Lookup<T>;

template<typename T, typename List>
concept UniqueType = concepts::TypeList<List> && List::template
UniqueType<T>;

template<typename List, typename T>
concept Contains = concepts::TypeList<List> && (Lookup<T, List> < Size<List>);
}
