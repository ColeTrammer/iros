#pragma once

#include <di/meta/list/list.h>

namespace di::meta {
namespace detail {
    template<typename T, typename U>
    struct ZipHelper : TypeConstant<List<>> {};

    template<typename T, typename U, typename... Ts, typename... Us>
    struct ZipHelper<List<T, Ts...>, List<U, Us...>>
        : TypeConstant<Concat<List<List<T, U>>, typename ZipHelper<List<Ts...>, List<Us...>>::Type>> {};
}

template<concepts::TypeList T, concepts::TypeList U>
requires(Size<T> == Size<U>)
using Zip = detail::ZipHelper<T, U>::Type;
}