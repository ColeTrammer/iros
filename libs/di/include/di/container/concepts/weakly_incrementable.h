#pragma once

#include "di/container/meta/iterator_ssize_type.h"
#include "di/meta/core.h"
#include "di/meta/language.h"
#include "di/meta/operations.h"

namespace di::concepts {
template<typename T>
concept WeaklyIncrementable = Movable<T> && requires(T iter) {
    typename meta::IteratorSSizeType<T>;
    requires SignedInteger<meta::IteratorSSizeType<T>>;
    { ++iter } -> SameAs<T&>;
    iter++;
};
}
