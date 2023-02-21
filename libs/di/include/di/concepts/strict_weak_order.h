#pragma once

#include <di/concepts/same_as.h>
#include <di/function/invoke.h>
#include <di/types/prelude.h>

namespace di::concepts {
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
