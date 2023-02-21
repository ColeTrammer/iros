#pragma once

#include <di/concepts/boolean_testable.h>
#include <di/concepts/copy_constructible.h>
#include <di/concepts/equality_comparable.h>
#include <di/concepts/move_constructible.h>

namespace di::concepts {
namespace detail {
    template<template<typename> typename>
    struct CheckTypeAliasExists;
}

template<typename T>
concept StoppableToken =
    CopyConstructible<T> && MoveConstructible<T> && requires(T const& token) {
                                                        { token.stop_requested() } -> BooleanTestable;
                                                        { token.stop_possible() } -> BooleanTestable;
                                                        typename detail::CheckTypeAliasExists<T::template CallbackType>;
                                                    };
}
