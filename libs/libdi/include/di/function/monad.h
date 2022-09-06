#pragma once

#include <di/concepts/derived_from.h>
#include <di/concepts/same_as.h>
#include <di/function/id.h>
#include <di/meta/remove_cvref.h>
#include <di/types/void.h>
#include <di/util/declval.h>
#include <di/util/forward.h>

namespace di::function::monad {
struct EnableMonad {};
}

namespace di::meta {
template<typename T>
using MonadUnitType = T::UnitType;
}

namespace di::concepts {
template<typename T>
concept MonadInstance = requires(T&& value) {
                            // unit (Haskell return)
                            typename meta::MonadUnitType<T>;
                            T(util::declval<meta::MonadUnitType<T>>());

                            // fmap (Haskell >>)
                            { util::forward<T>(value).transform(function::id) } -> SameAs<meta::RemoveCVRef<T>>;

                            // bind (Haskell >>=)
                            {
                                util::forward<T>(value).and_then([](auto&& value) {
                                    return T(util::forward<decltype(value)>(value));
                                })
                                } -> SameAs<meta::RemoveCVRef<T>>;
                        } && DerivedFrom<meta::RemoveCVRef<T>, function::monad::EnableMonad>;

template<template<typename...> typename T>
concept Monad = MonadInstance<decltype(T { types::Void {} })>;
}

namespace di::function::monad {
template<concepts::MonadInstance M, typename F>
constexpr auto operator%(M&& m, F&& f) -> decltype(util::forward<M>(m).transform(util::forward<F>(f))) {
    return util::forward<M>(m).transform(util::forward<F>(f));
}

template<concepts::MonadInstance M, typename F>
constexpr auto operator>>(M&& m, F&& f) -> decltype(util::forward<M>(m).and_then(util::forward<F>(f))) {
    return util::forward<M>(m).and_then(util::forward<F>(f));
}
}

namespace di::function {
template<template<typename...> typename M, typename T>
requires(concepts::Monad<M> && requires(T && value) { M { util::forward<T>(value) }; })
constexpr concepts::MonadInstance auto unit(T&& value) {
    return M { util::forward<T>(value) };
}
}
