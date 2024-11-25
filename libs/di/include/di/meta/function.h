#pragma once

#include <di/meta/core.h>

namespace di::concepts {
template<template<typename...> typename Fun, typename... Args>
concept ValidInstantiation = requires { typename Fun<Args...>; };
}

namespace di::meta {
namespace detail {
    template<template<typename...> typename, typename...>
    struct DeferHelper {};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsubobject-linkage"
    template<template<typename...> typename Fun, typename... Args>
    requires(concepts::ValidInstantiation<Fun, Args...>)
    struct DeferHelper<Fun, Args...> : TypeConstant<Fun<Args...>> {};
#pragma GCC diagnostic pop
}

template<template<typename...> typename Fun, typename... Args>
using Defer = detail::DeferHelper<Fun, Args...>;

template<template<typename...> typename Fun>
struct Quote {
    template<typename... Args>
    using Invoke = typename Defer<Fun, Args...>::Type;
};
}

namespace di::concepts {
template<typename T>
concept MetaInvocable = requires { typename meta::Quote<T::template Invoke>; };
}

namespace di::meta {
template<concepts::MetaInvocable Fun, typename... Args>
using Invoke = Type<Defer<Fun::template Invoke, Args...>>;

namespace detail {
    template<typename F, typename T>
    struct ApplyHelper;

    template<typename F, typename... Args>
    requires(concepts::ValidInstantiation<Invoke, F, Args...>)
    struct ApplyHelper<F, List<Args...>> : TypeConstant<Invoke<F, Args...>> {};
}

template<concepts::MetaInvocable F, concepts::TypeList T>
using Apply = Type<detail::ApplyHelper<F, T>>;

template<concepts::MetaInvocable Fun>
struct Uncurry {
    template<concepts::TypeList List>
    using Invoke = Apply<Fun, List>;
};

template<concepts::MetaInvocable MetaFn, typename... Bound>
struct BindFront {
    template<typename... Rest>
    using Invoke = meta::Invoke<MetaFn, Bound..., Rest...>;
};

template<concepts::MetaInvocable MetaFn, typename... Bound>
struct BindBack {
    template<typename... Rest>
    using Invoke = meta::Invoke<MetaFn, Rest..., Bound...>;
};

template<concepts::MetaInvocable MetaFn>
struct Flip {
    template<typename T, typename U>
    using Invoke = meta::Invoke<MetaFn, U, T>;
};

namespace detail {
    template<typename...>
    struct ComposeHelper;

    template<concepts::MetaInvocable F>
    struct ComposeHelper<F> : F {};

    template<concepts::MetaInvocable F, concepts::MetaInvocable G>
    struct ComposeHelper<F, G> {
        template<typename... Args>
        using Invoke = meta::Invoke<F, meta::Invoke<G, Args...>>;
    };

    template<concepts::MetaInvocable F, concepts::MetaInvocable... Gs>
    struct ComposeHelper<F, Gs...> : ComposeHelper<F, ComposeHelper<Gs...>> {};
}

template<concepts::MetaInvocable... Funs>
using Compose = detail::ComposeHelper<Funs...>;

namespace detail {
    template<typename...>
    struct ChainHelper;

    template<concepts::MetaInvocable F>
    struct ChainHelper<F> : F {};

    template<concepts::MetaInvocable F, concepts::MetaInvocable G>
    struct ChainHelper<F, G> {
        template<typename... Args>
        using Invoke = meta::Invoke<G, meta::Invoke<F, Args...>>;
    };

    template<concepts::MetaInvocable F, concepts::MetaInvocable... Gs>
    struct ChainHelper<F, Gs...> : ChainHelper<F, ChainHelper<Gs...>> {};
}

template<concepts::MetaInvocable... Funs>
using Chain = detail::ChainHelper<Funs...>;

template<typename T>
struct SameAs {
    template<typename... Args>
    using Invoke = Constexpr<(concepts::SameAs<T, Args> && ...)>;
};

template<concepts::MetaInvocable MetaFn>
struct Not {
    template<typename... Args>
    using Invoke = Constexpr<!meta::Invoke<MetaFn, Args...>::value>;
};
}
