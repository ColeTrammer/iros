#pragma once

#include <di/meta/integer_sequence.h>
#include <di/meta/list/list.h>

namespace di::meta {
namespace detail {
    template<typename T>
    struct AsListHelper {};

    template<typename T, T... values>
    struct AsListHelper<IntegerSequence<T, values...>> : TypeConstant<List<IntegralConstant<T, values>...>> {};

    template<template<typename...> typename Template, typename... Types>
    struct AsListHelper<Template<Types...>> : TypeConstant<List<Types...>> {};

    template<typename R, typename... Args>
    struct AsListHelper<R(Args...)> : TypeConstant<List<Args...>> {};
}

template<typename T>
using AsList = detail::AsListHelper<T>::Type;
}