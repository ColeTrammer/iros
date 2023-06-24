#pragma once

#include <di/meta/compare.h>
#include <di/meta/operations.h>

namespace di::concepts {
namespace detail {
    template<template<typename> typename>
    struct CheckTypeAliasExists;
}

template<typename T>
concept StoppableToken = CopyConstructible<T> && MoveConstructible<T> && requires(T const& token) {
    { token.stop_requested() } -> BooleanTestable;
    { token.stop_possible() } -> BooleanTestable;
    typename detail::CheckTypeAliasExists<T::template CallbackType>;
};
}
