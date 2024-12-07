#pragma once

#include "di/meta/core.h"
#include "di/types/partial_ordering.h"
#include "di/types/strong_ordering.h"
#include "di/types/weak_ordering.h"
#include "di/util/declval.h"

namespace di::meta {
namespace detail {
    template<typename... Types>
    struct CommonComparisonCategoryHelper : TypeConstant<void> {};

    template<>
    struct CommonComparisonCategoryHelper<> : TypeConstant<strong_ordering> {};

    template<typename... Types>
    struct CommonComparisonCategoryHelper<partial_ordering, Types...> : TypeConstant<partial_ordering> {};

    template<typename... Types>
    struct CommonComparisonCategoryHelper<weak_ordering, Types...>
        : TypeConstant<
              Conditional<concepts::SameAs<partial_ordering, typename CommonComparisonCategoryHelper<Types...>::Type>,
                          partial_ordering, weak_ordering>> {};

    template<typename... Types>
    struct CommonComparisonCategoryHelper<strong_ordering, Types...> : CommonComparisonCategoryHelper<Types...> {};
}

template<typename... Types>
using CommonComparisonCategory = Type<detail::CommonComparisonCategoryHelper<Types...>>;
}

namespace di::concepts {
namespace detail {
    template<typename T, typename Category>
    concept ComparesAs = SameAs<meta::CommonComparisonCategory<T, Category>, Category>;

    template<typename T, typename U>
    struct DefinitelyEqualityComparableWith {
        constexpr static bool value = false;
    };

    template<typename T, typename U>
    struct DefinitelyThreeWayComparableWith {
        using Type = void;
    };

    template<typename T, typename U>
    concept WeaklyEqualityComparableWith =
        (detail::DefinitelyEqualityComparableWith<T, U>::value) ||
        requires(meta::RemoveReference<T> const& a, meta::RemoveReference<U> const& b) {
            { a == b } -> SameAs<bool>;
            { a != b } -> SameAs<bool>;
            { b == a } -> SameAs<bool>;
            { b != a } -> SameAs<bool>;
        };

    template<typename T, typename U>
    concept PartiallyOrderedWith =
        (!concepts::SameAs<void, meta::Type<detail::DefinitelyThreeWayComparableWith<T, U>>>) ||
        requires(meta::RemoveReference<T> const& a, meta::RemoveReference<U> const& b) {
            { a < b } -> SameAs<bool>;
            { a > b } -> SameAs<bool>;
            { a <= b } -> SameAs<bool>;
            { a >= b } -> SameAs<bool>;
            { b < a } -> SameAs<bool>;
            { b > a } -> SameAs<bool>;
            { b <= a } -> SameAs<bool>;
            { b >= a } -> SameAs<bool>;
        };

    template<typename T, typename U, typename Category>
    concept WeaklyThreeWayComparableWith =
        requires(meta::RemoveReference<T> const& a, meta::RemoveReference<U> const& b) {
            { a <=> b } -> ComparesAs<Category>;
            { b <=> a } -> ComparesAs<Category>;
        };
}

template<typename T>
concept EqualityComparable =
    detail::DefinitelyEqualityComparableWith<T, T>::value || detail::WeaklyEqualityComparableWith<T, T>;

template<typename T, typename U>
concept EqualityComparableWith =
    detail::DefinitelyEqualityComparableWith<T, U>::value ||
    (EqualityComparable<T> && EqualityComparable<U> && detail::WeaklyEqualityComparableWith<T, U>);

template<typename T, typename Category = partial_ordering>
concept ThreeWayComparable =
    (detail::ComparesAs<meta::Type<detail::DefinitelyThreeWayComparableWith<T, T>>, Category>) ||
    (detail::WeaklyEqualityComparableWith<T, T> && detail::PartiallyOrderedWith<T, T> &&
     detail::WeaklyThreeWayComparableWith<T, T, Category>);

template<typename T, typename U, typename Category = partial_ordering>
concept ThreeWayComparableWith =
    (detail::ComparesAs<meta::Type<detail::DefinitelyThreeWayComparableWith<T, T>>, Category>) ||
    (ThreeWayComparable<T> && ThreeWayComparable<U> && detail::WeaklyEqualityComparableWith<T, U> &&
     detail::PartiallyOrderedWith<T, U> && detail::WeaklyThreeWayComparableWith<T, U, Category>);

template<typename T>
concept TotallyOrdered = ThreeWayComparable<T, weak_ordering>;

template<typename T, typename U>
concept TotallyOrderedWith = ThreeWayComparableWith<T, U, weak_ordering>;
}

namespace di::meta {
namespace detail {
    template<typename T, typename U>
    struct CompareThreeWayResultHelper : meta::TypeConstant<void> {};

    template<typename T, typename U>
    requires(concepts::LanguageVoid<Type<concepts::detail::DefinitelyThreeWayComparableWith<T, U>>> &&
             requires {
                 {
                     util::declval<meta::RemoveReference<T> const&>() <=>
                         util::declval<meta::RemoveReference<U> const&>()
                 };
             })
    struct CompareThreeWayResultHelper<T, U>
        : meta::TypeConstant<decltype(util::declval<meta::RemoveReference<T> const&>() <=>
                                      util::declval<meta::RemoveReference<U> const&>())> {};

    template<typename T, typename U>
    requires(!concepts::LanguageVoid<meta::Type<concepts::detail::DefinitelyThreeWayComparableWith<T, U>>>)
    struct CompareThreeWayResultHelper<T, U>
        : meta::TypeConstant<meta::Type<concepts::detail::DefinitelyThreeWayComparableWith<T, U>>> {};
}

template<typename T, typename U = T>
using CompareThreeWayResult = meta::Type<detail::CompareThreeWayResultHelper<T, U>>;
}
