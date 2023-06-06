#pragma once

#include <di/meta/constexpr.h>
#include <di/meta/list/list.h>
#include <di/meta/list/list_v.h>

namespace di::meta {
namespace detail {
    template<typename T>
    struct AsListHelper {};

    template<auto... values>
    struct AsListHelper<ListV<values...>> : TypeConstant<List<Constexpr<values>...>> {};

    template<template<typename...> typename Template, typename... Types>
    struct AsListHelper<Template<Types...>> : TypeConstant<List<Types...>> {};

    template<typename R, typename... Args>
    struct AsListHelper<R(Args...)> : TypeConstant<List<Args...>> {};
}

template<typename T>
using AsList = detail::AsListHelper<T>::Type;
}
