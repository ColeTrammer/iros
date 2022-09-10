#pragma once

#include <di/concepts/same_as.h>
#include <di/function/id.h>
#include <di/function/monad/monad_bind.h>
#include <di/function/monad/monad_enable.h>
#include <di/function/monad/monad_fmap.h>
#include <di/meta/remove_cvref.h>
#include <di/types/void.h>
#include <di/util/forward.h>

namespace di::concepts {
namespace detail {
    template<typename T>
    struct MonadBindId {
        constexpr T operator()(auto&& value) const { return T(util::forward<decltype(value)>(value)); }
        constexpr T operator()() const { return T(); }
    };
}

template<typename T>
concept MonadInstance = requires(T&& value) {
                            // fmap (Haskell >>)
                            { function::monad::fmap(util::forward<T>(value), function::id) } -> SameAs<meta::RemoveCVRef<T>>;

                            // bind (Haskell >>=)
                            {
                                function::monad::bind(util::forward<T>(value), detail::MonadBindId<meta::RemoveCVRef<T>> {})
                                } -> SameAs<meta::RemoveCVRef<T>>;
                        } && function::monad::enable_monad(types::in_place_type<meta::RemoveCVRef<T>>);

template<template<typename...> typename T>
concept Monad = MonadInstance<decltype(T { types::Void {} })>;
}
