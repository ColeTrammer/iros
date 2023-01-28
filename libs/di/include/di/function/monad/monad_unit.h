#pragma once

#include <di/function/monad/monad_concept.h>
#include <di/util/forward.h>

namespace di::function::monad {
template<template<typename...> typename M, typename T>
requires(concepts::Monad<M> && requires(T && value) { M { util::forward<T>(value) }; })
constexpr concepts::MonadInstance auto unit(T&& value) {
    return M { util::forward<T>(value) };
}
}
