#pragma once

#include <di/function/invoke.h>

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

    template<typename R, typename T, typename U>
    concept InvocableToStrictWeakOrder = Invocable<R, T, U> && StrictWeakOrderCategory<meta::InvokeResult<R, T, U>>;
}

template<typename R, typename T, typename U = T>
concept StrictWeakOrder = detail::InvocableToStrictWeakOrder<R, T, T> && detail::InvocableToStrictWeakOrder<R, T, U> &&
                          detail::InvocableToStrictWeakOrder<R, U, T> && detail::InvocableToStrictWeakOrder<R, U, U>;
}
