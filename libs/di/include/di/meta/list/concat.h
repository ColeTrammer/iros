#pragma once

#include <di/meta/list/list.h>

namespace di::meta {
namespace detail {
    template<typename...>
    struct ConcatHelper {};

    template<typename... Ts, typename... Us, typename... Rest>
    struct ConcatHelper<List<Ts...>, List<Us...>, Rest...> : ConcatHelper<List<Ts..., Us...>, Rest...> {};

    template<typename T>
    struct ConcatHelper<T> : TypeConstant<T> {};

    template<>
    struct ConcatHelper<> : TypeConstant<List<>> {};
}

template<concepts::TypeList... Lists>
using Concat = detail::ConcatHelper<Lists...>::Type;
}