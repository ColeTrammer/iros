#pragma once

#include <di/meta/list/invoke.h>

namespace di::meta {
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
}