#pragma once

#include <di/concepts/movable.h>
#include <di/concepts/same_as.h>
#include <di/concepts/signed_integer.h>
#include <di/container/meta/iterator_ssize_type.h>

namespace di::concepts {
template<typename T>
concept WeaklyIncrementable = Movable<T> && requires(T iter) {
    typename meta::IteratorSSizeType<T>;
    requires SignedInteger<meta::IteratorSSizeType<T>>;
    { ++iter } -> SameAs<T&>;
    iter++;
};
}
