#pragma once

#include <di/concepts/same_as.h>
#include <di/function/identity.h>
#include <di/function/monad/monad_bind.h>
#include <di/function/monad/monad_enable.h>
#include <di/function/monad/monad_fmap.h>
#include <di/meta/list/prelude.h>
#include <di/meta/remove_cvref.h>
#include <di/types/void.h>
#include <di/util/forward.h>
#include <di/util/reference_wrapper.h>

namespace di::concepts {
namespace detail {
    template<typename T>
    struct MonadBindId {
        constexpr T operator()(auto&& value) const { return T(util::forward<decltype(value)>(value)); }
        constexpr T operator()() const { return T(); }
    };

    template<typename M>
    struct MonadValue {};

    template<template<typename...> typename Monad, typename T, typename... Args>
    struct MonadValue<Monad<T, Args...>> : meta::TypeConstant<T> {};

    template<typename M>
    struct MonadFmapId {
        template<typename T>
        requires(!concepts::LValueReference<T> || !concepts::LValueReference<meta::Type<MonadValue<M>>>)
        constexpr auto operator()(T&& value) const {
            return util::forward<T>(value);
        }

        constexpr auto operator()(auto& value) const
        requires(concepts::LValueReference<meta::Type<MonadValue<M>>>)
        {
            return util::ref(value);
        }

        constexpr void operator()() const {}
    };
}

template<typename T>
concept MonadInstance = requires(T&& value) {
                            // fmap (Haskell >>)
                            {
                                function::monad::fmap(util::forward<T>(value),
                                                      detail::MonadFmapId<meta::RemoveCVRef<T>> {})
                                } -> SameAs<meta::RemoveCVRef<T>>;

                            // bind (Haskell >>=)
                            {
                                function::monad::bind(util::forward<T>(value),
                                                      detail::MonadBindId<meta::RemoveCVRef<T>> {})
                                } -> SameAs<meta::RemoveCVRef<T>>;
                        } && function::monad::enable_monad(types::in_place_type<meta::RemoveCVRef<T>>);

template<template<typename...> typename T>
concept Monad = MonadInstance<decltype(T { types::Void {} })>;
}
