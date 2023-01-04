#pragma once

#include <di/meta/list/concepts/valid_instantiation.h>

namespace di::meta {
namespace detail {
    template<template<typename...> typename, typename...>
    struct DeferHelper {};

    template<template<typename...> typename Fun, typename... Args>
    requires(concepts::ValidInstantiation<Fun, Args...>)
    struct DeferHelper<Fun, Args...> : TypeConstant<Fun<Args...>> {};
}

template<template<typename...> typename Fun, typename... Args>
struct Defer : detail::DeferHelper<Fun, Args...> {};
}