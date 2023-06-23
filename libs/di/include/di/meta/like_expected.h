#pragma once

#include <di/concepts/language_void.h>
#include <di/meta/core.h>
#include <di/vocab/expected/expected_forward_declaration.h>

namespace di::meta {
namespace detail {
    template<typename T, typename U>
    struct LikeExpectedHelper : TypeConstant<U> {};

    template<typename X, typename E, typename U>
    requires(!concepts::LanguageVoid<E>)
    struct LikeExpectedHelper<vocab::Expected<X, E>, U> : TypeConstant<vocab::Expected<U, E>> {};

    template<typename X, typename E, typename U>
    requires(concepts::LanguageVoid<E>)
    struct LikeExpectedHelper<vocab::Expected<X, E>, U> : TypeConstant<U> {};
}

template<typename T, typename U>
using LikeExpected = detail::LikeExpectedHelper<T, U>::Type;
}
