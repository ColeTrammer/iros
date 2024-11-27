#pragma once

#include <di/function/compose.h>
#include <di/function/invoke.h>
#include <di/function/pipeable.h>
#include <di/util/forward.h>

namespace di::function::pipeline {
template<typename T, Pipeable F>
requires(concepts::Invocable<F, T>)
constexpr auto operator|(T&& value, F&& function) -> decltype(auto) {
    return function::invoke(util::forward<F>(function), util::forward<T>(value));
}

template<Pipeable F, Pipeable G>
requires(!concepts::Invocable<G, F>)
constexpr auto operator|(F&& f, G&& g) {
    return function::compose(util::forward<G>(g), util::forward<F>(f));
}

template<Pipeable F, Pipeable G>
constexpr auto operator*(F&& f, G&& g) {
    return function::compose(util::forward<G>(g), util::forward<F>(f));
}
}
