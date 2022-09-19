#pragma once

namespace di::meta {
namespace detail {
    template<typename... Types>
    struct CommonTypeHelper {};

    template<typename T, typename... Rest>
    struct CommonTypeHelper<T, Rest...> {
        using Type = T;
    };
}

template<typename... Types>
using CommonType = detail::CommonTypeHelper<Types...>::Type;
}