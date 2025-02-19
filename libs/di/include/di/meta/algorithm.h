#pragma once

#include "di/meta/constexpr.h"
#include "di/meta/core.h"
#include "di/meta/function.h"
#include "di/meta/list.h"
#include "di/types/integers.h"
#include "di/vocab/tuple/tuple_forward_declaration.h"

namespace di::meta {
namespace detail {
    template<typename Pred, typename Type>
    struct AllHelper {};

    template<typename Pred, typename... Types>
    struct AllHelper<Pred, List<Types...>> : Constexpr<(meta::Invoke<Pred, Types> {} && ...)> {};
}

template<concepts::TypeList List, concepts::MetaInvocable Pred>
constexpr inline bool All = detail::AllHelper<Pred, List>::value;

namespace detail {
    template<typename R, typename List>
    struct AsLanguageFunction;

    template<typename R, typename... Types>
    struct AsLanguageFunction<R, List<Types...>> : TypeConstant<R(Types...)> {};
}

template<typename R, concepts::TypeList T>
using AsLanguageFunction = Type<detail::AsLanguageFunction<R, T>>;

namespace detail {
    template<typename T>
    struct AsListHelper {};

    template<auto... values>
    struct AsListHelper<ListV<values...>> : TypeConstant<List<Constexpr<values>...>> {};

    template<template<typename...> typename Template, typename... Types>
    struct AsListHelper<Template<Types...>> : TypeConstant<List<Types...>> {};

    template<typename R, typename... Args>
    struct AsListHelper<R(Args...)> : TypeConstant<List<Args...>> {};
}

template<typename T>
using AsList = Type<detail::AsListHelper<T>>;

namespace detail {
    template<template<typename...> typename Template, typename List>
    struct AsTemplateHelper {};

    template<template<typename...> typename Template, typename... Types>
    requires(concepts::ValidInstantiation<Template, Types...>)
    struct AsTemplateHelper<Template, List<Types...>> : TypeConstant<Template<Types...>> {};
}

template<template<typename...> typename Template, concepts::TypeList T>
using AsTemplate = Type<detail::AsTemplateHelper<Template, T>>;

template<concepts::TypeList T>
using AsTuple = AsTemplate<vocab::Tuple, T>;

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
using Concat = Type<detail::ConcatHelper<Lists...>>;

template<concepts::TypeList L, typename T>
using PushFront = Concat<List<T>, L>;

template<concepts::TypeList L, typename T>
using PushBack = Concat<L, List<T>>;

template<concepts::TypeList List>
using Join = Apply<Quote<Concat>, List>;

namespace detail {
    template<typename T>
    struct PopFrontHelper : TypeConstant<List<>> {};

    template<typename T, typename... Rest>
    struct PopFrontHelper<List<T, Rest...>> : TypeConstant<List<Rest...>> {};
}

template<concepts::TypeList L>
using PopFront = Type<detail::PopFrontHelper<L>>;

namespace detail {
    template<typename T>
    struct PopBackHelper : TypeConstant<List<>> {};

    template<typename T, typename U, typename... Rest>
    struct PopBackHelper<List<T, U, Rest...>> : TypeConstant<PushFront<Type<PopBackHelper<List<U, Rest...>>>, T>> {};
}

template<concepts::TypeList L>
using PopBack = Type<detail::PopBackHelper<L>>;

namespace detail {
    template<typename List, typename Acc, typename MetaFn>
    struct FoldHelper {};

    template<typename Acc, typename MetaFn>
    struct FoldHelper<List<>, Acc, MetaFn> : TypeConstant<Acc> {};

    template<typename T, typename... Rest, typename Acc, typename MetaFn>
    struct FoldHelper<List<T, Rest...>, Acc, MetaFn>
        : TypeConstant<Type<FoldHelper<List<Rest...>, Invoke<MetaFn, Acc, T>, MetaFn>>> {};
}

template<concepts::TypeList List, typename Init, concepts::MetaInvocable MetaFn>
using Fold = Type<detail::FoldHelper<List, Init, MetaFn>>;

namespace detail {
    template<typename List, typename Init, typename MetaFn>
    struct FoldRightHelper {};

    template<typename Init, typename MetaFn>
    struct FoldRightHelper<List<>, Init, MetaFn> : TypeConstant<Init> {};

    template<typename T, typename... Rest, typename Init, typename MetaFn>
    struct FoldRightHelper<List<T, Rest...>, Init, MetaFn>
        : TypeConstant<Invoke<MetaFn, Type<FoldRightHelper<List<Rest...>, Init, MetaFn>>, T>> {};
}

template<concepts::TypeList List, typename Init, concepts::MetaInvocable MetaFn>
using FoldRight = Type<detail::FoldRightHelper<List, Init, MetaFn>>;

namespace detail {
    template<typename Pred>
    struct FilterReducer {
        template<typename Acc, typename Val>
        using Invoke = Conditional<Invoke<Pred, Val>::value, PushBack<Acc, Val>, Acc>;
    };
}

template<concepts::TypeList List, concepts::MetaInvocable Pred>
using Filter = Fold<List, meta::List<>, detail::FilterReducer<Pred>>;

namespace detail {
    template<typename Needle, typename Replacement>
    struct ReplaceReducer {
        template<typename Acc, typename Val>
        using Invoke = PushBack<Acc, Conditional<concepts::SameAs<Val, Needle>, Replacement, Val>>;
    };
}

template<concepts::TypeList List, typename Needle, typename Replacement>
using Replace = Fold<List, meta::List<>, detail::ReplaceReducer<Needle, Replacement>>;

namespace detail {
    template<typename Pred, typename Replacement>
    struct ReplaceIfReducer {
        template<typename Acc, typename Val>
        using Invoke = PushBack<Acc, Conditional<Invoke<Pred, Val>::value, Replacement, Val>>;
    };
}

template<concepts::TypeList List, concepts::MetaInvocable Pred, typename Replacement>
using ReplaceIf = Fold<List, meta::List<>, detail::ReplaceIfReducer<Pred, Replacement>>;

namespace detail {
    template<typename...>
    struct TransformHelper {};

    template<typename... Types, typename Fun>
    requires(concepts::MetaInvocable<Fun> && (concepts::ValidInstantiation<Invoke, Fun, Types> && ...))
    struct TransformHelper<List<Types...>, Fun> : TypeConstant<List<Invoke<Fun, Types>...>> {};
}

template<concepts::TypeList List, typename Function>
using Transform = detail::TransformHelper<List, Function>::Type;

namespace detail {
    struct PushBackIfUnique {
        template<concepts::TypeList Lst, typename T>
        struct Impl : TypeConstant<PushBack<Lst, T>> {};

        template<concepts::TypeList Lst, typename T>
        requires(Contains<Lst, T>)
        struct Impl<Lst, T> : TypeConstant<Lst> {};

        template<concepts::TypeList Lst, typename T>
        using Invoke = Type<Impl<Lst, T>>;
    };
}

template<concepts::TypeList Lst>
using Unique = Fold<Lst, List<>, detail::PushBackIfUnique>;

namespace detail {
    template<typename T, typename U>
    struct ZipHelper : TypeConstant<List<>> {};

    template<typename T, typename U, typename... Ts, typename... Us>
    struct ZipHelper<List<T, Ts...>, List<U, Us...>>
        : TypeConstant<Concat<List<List<T, U>>, typename ZipHelper<List<Ts...>, List<Us...>>::Type>> {};
}

template<concepts::TypeList T, concepts::TypeList U>
requires(Size<T> == Size<U>)
using Zip = Type<detail::ZipHelper<T, U>>;

namespace detail {
    template<typename T, usize N>
    struct RepeatHelper;

    template<typename T>
    struct RepeatHelper<T, 0> : TypeConstant<List<>> {};

    template<typename T>
    struct RepeatHelper<T, 1> : TypeConstant<List<T>> {};

    template<typename T, usize N>
    requires(N > 1)
    struct RepeatHelper<T, N>
        : TypeConstant<Concat<Type<RepeatHelper<T, N / 2>>, Type<RepeatHelper<T, (N + 1) / 2>>>> {};
}

template<typename T, usize N>
using Repeat = Type<detail::RepeatHelper<T, N>>;

namespace detail {
    template<typename... Types>
    struct CartesianProductHelper {};

    template<>
    struct CartesianProductHelper<> : TypeConstant<List<List<>>> {};

    template<typename... Types>
    struct CartesianProductHelper<List<Types...>> : TypeConstant<List<List<Types>...>> {};

    template<typename... Ts, typename... Rest>
    struct CartesianProductHelper<List<Ts...>, Rest...>
        : TypeConstant<Concat<Transform<Type<CartesianProductHelper<Rest...>>, BindBack<Quote<PushFront>, Ts>>...>> {};
}

template<concepts::TypeList... Types>
using CartesianProduct = Type<detail::CartesianProductHelper<Types...>>;

namespace detail {
    template<typename...>
    struct ConcatVHelper {};

    template<auto... ts, auto... us, typename... Rest>
    struct ConcatVHelper<ListV<ts...>, ListV<us...>, Rest...> : ConcatVHelper<ListV<ts..., us...>, Rest...> {};

    template<typename T>
    struct ConcatVHelper<T> : TypeConstant<T> {};

    template<>
    struct ConcatVHelper<> : TypeConstant<ListV<>> {};
}

template<concepts::InstanceOfV<ListV>... Lists>
using ConcatV = Type<detail::ConcatVHelper<Lists...>>;

namespace detail {
    template<typename T>
    struct ReverseVHelper {};

    template<>
    struct ReverseVHelper<ListV<>> : TypeConstant<ListV<>> {};

    template<auto v>
    struct ReverseVHelper<ListV<v>> : TypeConstant<ListV<v>> {};

    template<auto v, auto... vs>
    struct ReverseVHelper<ListV<v, vs...>> : TypeConstant<ConcatV<Type<ReverseVHelper<ListV<vs...>>>, ListV<v>>> {};
}

template<concepts::InstanceOfV<ListV> List>
using ReverseV = Type<detail::ReverseVHelper<List>>;

// To produce an integer sequence using a minimal number of template
// instantiations, we partition the problem into halves, and construct
// the correcct sequence using the Concat helper.
namespace detail {
    template<typename T, typename X, typename Y>
    struct MakeIntegerSequenceConcatHelper;

    template<typename T, T... s1, T... s2>
    struct MakeIntegerSequenceConcatHelper<T, ListV<s1...>, ListV<s2...>> {
        using Type = ListV<s1..., (T(sizeof...(s1) + s2))...>;
    };

    template<typename T, usize count>
    struct MakeIntegerSequenceHelper {
        using A = meta::Type<MakeIntegerSequenceHelper<T, count / 2>>;
        using B = meta::Type<MakeIntegerSequenceHelper<T, count - count / 2>>;
        using Type = meta::Type<MakeIntegerSequenceConcatHelper<T, A, B>>;
    };

    template<typename T>
    struct MakeIntegerSequenceHelper<T, 1> : TypeConstant<ListV<T(0)>> {};

    template<typename T>
    struct MakeIntegerSequenceHelper<T, 0> : TypeConstant<ListV<>> {};
}

template<typename T, usize count>
using MakeIntegerSequence = Type<detail::MakeIntegerSequenceHelper<T, count>>;

template<usize count>
using MakeIndexSequence = MakeIntegerSequence<usize, count>;

template<usize count>
using MakeReverseIndexSequence = ReverseV<MakeIntegerSequence<usize, count>>;

template<typename... Types>
using IndexSequenceFor = MakeIndexSequence<sizeof...(Types)>;
}
