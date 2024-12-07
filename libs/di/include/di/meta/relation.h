#pragma once

#include "di/function/invoke.h"

namespace di::concepts {
template<typename F, typename... Args>
concept Predicate = InvocableTo<F, bool, Args...>;

template<typename R, typename T, typename U>
concept Relation = Predicate<R, T, T> && Predicate<R, U, U> && Predicate<R, T, U> && Predicate<R, U, T>;

template<typename R, typename T, typename U>
concept EquivalenceRelation = Relation<R, T, U>;

namespace detail {
    template<typename Cat>
    concept StrictWeakOrderCategory = SameAs<Cat, strong_ordering> || SameAs<Cat, weak_ordering>;

    template<typename Cat>
    concept ComparisonCategory =
        SameAs<Cat, strong_ordering> || SameAs<Cat, weak_ordering> || SameAs<Cat, partial_ordering>;

    template<typename R, typename T, typename U>
    concept InvocableToStrictWeakOrder = Invocable<R, T, U> && StrictWeakOrderCategory<meta::InvokeResult<R, T, U>>;

    template<typename R, typename T, typename U>
    concept InvocableToComparisonCategory = Invocable<R, T, U> && ComparisonCategory<meta::InvokeResult<R, T, U>>;
}

template<typename R, typename T, typename U = T>
concept StrictWeakOrder = detail::InvocableToStrictWeakOrder<R, T, T> && detail::InvocableToStrictWeakOrder<R, T, U> &&
                          detail::InvocableToStrictWeakOrder<R, U, T> && detail::InvocableToStrictWeakOrder<R, U, U>;

template<typename R, typename T, typename U = T>
concept StrictPartialOrder =
    detail::InvocableToComparisonCategory<R, T, T> && detail::InvocableToComparisonCategory<R, T, U> &&
    detail::InvocableToComparisonCategory<R, U, T> && detail::InvocableToComparisonCategory<R, U, U>;
}
