#pragma once

#include <di/meta/constexpr.h>
#include <di/meta/core.h>
#include <di/meta/function.h>
#include <di/types/integers.h>

namespace di::meta {
namespace detail {
    template<usize index, typename... Types>
    struct AtHelper {};

    template<typename T, typename... Rest>
    struct AtHelper<0, T, Rest...> : TypeConstant<T> {};

    template<usize index, typename T, typename... Rest>
    struct AtHelper<index, T, Rest...> : AtHelper<index - 1, Rest...> {};
}

namespace detail {
    template<typename Needle, typename... Types>
    struct LookupHelper : Constexpr<0ZU> {};

    template<typename Needle, typename T, typename... Rest>
    struct LookupHelper<Needle, T, Rest...>
        : Constexpr<concepts::SameAs<T, Needle> ? 0ZU : 1 + LookupHelper<Needle, Rest...>::value> {};
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

    using Back = Type<detail::BackHelper<Rest...>>;

    constexpr static usize size = sizeof...(Rest) + 1;

    template<usize index>
    using At = Type<detail::AtHelper<index, Head, Rest...>>;

    template<typename T>
    constexpr static auto Lookup = detail::LookupHelper<T, Head, Rest...>::value;

    template<typename T>
    constexpr static bool UniqueType =
        (static_cast<usize>(concepts::SameAs<T, Head>) + ... + static_cast<usize>(concepts::SameAs<T, Rest>)) == 1ZU;
};

template<typename T>
struct List<T> {
    using Front = T;
    using Back = T;

    constexpr static usize size = 1;

    template<usize index>
    using At = detail::AtHelper<index, T>::Type;

    template<typename U>
    constexpr static auto Lookup = detail::LookupHelper<U, T>::value;

    template<typename U>
    constexpr static bool UniqueType = concepts::SameAs<T, U>;
};

template<>
struct List<> {
    constexpr static usize size = 0;

    template<typename U>
    constexpr static bool UniqueType = false;

    template<usize index>
    requires(false)
    using At = void;

    template<usize index>
    requires(false)
    using Front = void;

    template<usize index>
    requires(false)
    using Back = void;

    template<typename U>
    constexpr static usize Lookup = 0;
};

template<concepts::TypeList T>
using Front = T::Front;

template<concepts::TypeList T>
using Back = T::Back;

template<concepts::TypeList T>
constexpr inline usize Size = T::size;

template<concepts::TypeList T, usize index>
requires(index < Size<T>)
using At = typename T::template At<index>;

template<typename T, concepts::TypeList List>
constexpr static inline auto Lookup = List::template Lookup<T>;

template<typename T, typename List>
concept UniqueType = concepts::TypeList<List> && List::template UniqueType<T>;

template<typename List, typename T>
concept Contains = concepts::TypeList<List> && (Lookup<T, List> < Size<List>);

namespace detail {
    template<typename T, typename List>
    struct CountHelper;

    template<typename T>
    struct CountHelper<T, List<>> : meta::Constexpr<0ZU> {};

    template<typename T, typename U, typename... Rest>
    struct CountHelper<T, List<U, Rest...>> {
        constexpr static auto value = (concepts::SameAs<T, U> ? 1 : 0) + CountHelper<T, List<Rest...>>::value;
    };
}

template<concepts::TypeList List, typename T>
constexpr static auto Count = detail::CountHelper<T, List>::value;

template<typename List, typename T>
concept ExactlyOnce = concepts::TypeList<List> && Count<List, T> == 1;
}
