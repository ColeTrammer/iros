#pragma once

#include <di/meta/false_type.h>
#include <di/meta/integer_sequence.h>
#include <di/meta/integral_constant.h>
#include <di/meta/true_type.h>
#include <di/meta/type_constant.h>
#include <di/types/prelude.h>

namespace di::meta {
template<typename...>
struct List;
}

namespace di::concepts {
namespace detail {
    template<typename T>
    struct TypeListHelper : meta::FalseType {};

    template<typename... Types>
    struct TypeListHelper<meta::List<Types...>> : meta::TrueType {};
}

template<typename T>
concept TypeList = detail::TypeListHelper<T>::value;
}

namespace di::meta {
namespace detail {
    template<types::size_t index, typename... Types>
    struct AtHelper {};

    template<typename T, typename... Rest>
    struct AtHelper<0, T, Rest...> : TypeConstant<T> {};

    template<types::size_t index, typename T, typename... Rest>
    struct AtHelper<index, T, Rest...> : AtHelper<index - 1, Rest...> {};
}

template<typename Head, typename... Rest>
struct List<Head, Rest...> {
    using Front = Head;

    constexpr static size_t size = sizeof...(Rest) + 1;

    template<size_t index>
    using At = detail::AtHelper<index, Head, Rest...>::Type;
};

template<typename T>
struct List<T> {
    using Front = T;

    constexpr static size_t size = 1;

    template<size_t index>
    using At = detail::AtHelper<index, T>::Type;
};

template<concepts::TypeList T>
using Front = T::Front;

template<concepts::TypeList T>
constexpr inline size_t Size = T::size;

template<concepts::TypeList T, size_t index>
requires(index < Size<T>)
using At = typename T::At<index>;

namespace detail {
    template<typename...>
    struct ConcatHelper {};

    template<typename... Ts, typename... Us, typename... Rest>
    struct ConcatHelper<List<Ts...>, List<Us...>, Rest...> : ConcatHelper<List<Ts..., Us...>, Rest...> {};

    template<typename T>
    struct ConcatHelper<T> : TypeConstant<T> {};

    template<>
    struct ConcatHelper<> : TypeConstant<List<>> {};
}

template<concepts::TypeList... Lists>
using Concat = detail::ConcatHelper<Lists...>::Type;

namespace detail {
    template<typename T>
    struct AsListHelper {};

    template<typename T, T... values>
    struct AsListHelper<IntegerSequence<T, values...>> : TypeConstant<List<IntegralConstant<T, values>...>> {};
}

template<typename T>
using AsList = detail::AsListHelper<T>::Type;

namespace detail {
    template<typename T, typename U>
    struct ZipHelper : TypeConstant<List<>> {};

    template<typename T, typename U, typename... Ts, typename... Us>
    struct ZipHelper<List<T, Ts...>, List<U, Us...>>
        : TypeConstant<Concat<List<List<T, U>>, typename ZipHelper<List<Ts...>, List<Us...>>::Type>> {};
}

template<concepts::TypeList T, concepts::TypeList U>
requires(Size<T> == Size<U>)
using Zip = detail::ZipHelper<T, U>::Type;
}
